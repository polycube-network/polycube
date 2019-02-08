/*
 * Copyright 2018 The Polycube Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <uapi/linux/bpf.h>
#include <uapi/linux/filter.h>
#include <uapi/linux/icmp.h>
#include <uapi/linux/if_arp.h>
#include <uapi/linux/if_ether.h>
#include <uapi/linux/if_packet.h>
#include <uapi/linux/in.h>
#include <uapi/linux/ip.h>
#include <uapi/linux/pkt_cls.h>
#include <uapi/linux/tcp.h>
#include <uapi/linux/udp.h>

#define NAT_MAP_DIM 32768

#define IP_CSUM_OFFSET (sizeof(struct eth_hdr) + offsetof(struct iphdr, check))
#define UDP_CSUM_OFFSET                            \
  (sizeof(struct eth_hdr) + sizeof(struct iphdr) + \
   offsetof(struct udphdr, check))
#define TCP_CSUM_OFFSET                            \
  (sizeof(struct eth_hdr) + sizeof(struct iphdr) + \
   offsetof(struct tcphdr, check))
#define ICMP_CSUM_OFFSET                           \
  (sizeof(struct eth_hdr) + sizeof(struct iphdr) + \
   offsetof(struct icmphdr, checksum))
#define IS_PSEUDO 0x10

struct eth_hdr {
  __be64 dst : 48;
  __be64 src : 48;
  __be16 proto;
} __attribute__((packed));

// Session table
struct st_k {
  uint32_t src_ip;
  uint32_t dst_ip;
  uint16_t src_port;
  uint16_t dst_port;
  uint8_t proto;
};
struct st_v {
  uint32_t new_ip;
  uint16_t new_port;
  uint8_t originating_rule_type;
};
BPF_TABLE("lru_hash", struct st_k, struct st_v, egress_session_table,
          NAT_MAP_DIM);
BPF_TABLE("lru_hash", struct st_k, struct st_v, ingress_session_table,
          NAT_MAP_DIM);

// SNAT + MASQUERADE rules
struct sm_k {
  u32 internal_netmask_len;
  __be32 internal_ip;
};
struct sm_v {
  __be32 external_ip;
  uint8_t entry_type;
};

BPF_F_TABLE("lpm_trie", struct sm_k, struct sm_v, sm_rules, 1024,
            BPF_F_NO_PREALLOC);

// DNAT + PORTFORWARDING rules
struct dp_k {
  u32 mask;
  __be32 external_ip;
  __be16 external_port;
  uint8_t proto;
};
struct dp_v {
  __be32 internal_ip;
  __be16 internal_port;
  uint8_t entry_type;
};
BPF_F_TABLE("lpm_trie", struct dp_k, struct dp_v, dp_rules, 1024,
            BPF_F_NO_PREALLOC);

// Port numbers
BPF_TABLE("array", u32, u16, first_free_port, 1);

static inline __be16 get_free_port() {
  u32 i = 0;
  u16 *new_port_p = first_free_port.lookup(&i);
  if (!new_port_p)
    return 0;
  rcu_read_lock();
  if (*new_port_p < 1024 || *new_port_p == 65535)
    *new_port_p = 1024;
  *new_port_p = *new_port_p + 1;
  rcu_read_unlock();
  return bpf_htons(*new_port_p);
}

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  // NAT processing happens in 4 steps:
  // 1) packet parsing
  // 2) session table lookup
  // 3) rule lookup
  // 4) packet modification

  // Parse packet
  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;

  struct eth_hdr *eth = data;
  if (data + sizeof(*eth) > data_end)
    goto DROP;

  pcn_log(
      ctx, LOG_TRACE,
      "Received new packet: in_port: %d eth_type: 0x%x mac_src: %M mac_dst: %M",
      md->in_port, bpf_ntohs(eth->proto), eth->src, eth->dst);

  switch (eth->proto) {
  case htons(ETH_P_IP):
    // Packet is IP
    pcn_log(ctx, LOG_DEBUG, "Received IP Packet");
    break;
  case htons(ETH_P_ARP):
    // Packet is ARP: act as a transparent NAT and forward the packet to the
    // opposite interface
    if (md->in_port == EXTERNAL_PORT) {
      pcn_log(ctx, LOG_TRACE,
              "Received ARP Packet from the outside network, forwarding on "
              "internal port");
      return pcn_pkt_redirect(ctx, md, INTERNAL_PORT);
    } else {
      pcn_log(ctx, LOG_TRACE,
              "Received ARP Packet from the inside network, forwarding on "
              "external port");
      return pcn_pkt_redirect(ctx, md, EXTERNAL_PORT);
    }
    break;
  default:
    pcn_log(ctx, LOG_TRACE, "Unknown eth proto: %d, dropping",
            bpf_ntohs(eth->proto));
    goto DROP;
  }

  // Packet data
  uint32_t srcIp = 0;
  uint32_t dstIp = 0;
  uint16_t srcPort = 0;
  uint16_t dstPort = 0;
  uint8_t proto = 0;

  // Nat data
  uint32_t newIp = 0;
  uint16_t newPort = 0;
  uint8_t rule_type = 0;

  // Status data
  uint8_t update_session_table = 1;

  struct iphdr *ip = data + sizeof(*eth);
  if (data + sizeof(*eth) + sizeof(*ip) > data_end)
    goto DROP;

  pcn_log(ctx, LOG_TRACE, "Processing IP packet: src %I, dst: %I", ip->saddr,
          ip->daddr);

  srcIp = ip->saddr;
  dstIp = ip->daddr;
  proto = ip->protocol;

  switch (ip->protocol) {
  case IPPROTO_TCP: {
    struct tcphdr *tcp = data + sizeof(*eth) + sizeof(*ip);
    if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*tcp) > data_end)
      goto DROP;

    pcn_log(ctx, LOG_TRACE, "Packet is TCP: src_port %P, dst_port %P",
            tcp->source, tcp->dest);
    srcPort = tcp->source;
    dstPort = tcp->dest;
    break;
  }
  case IPPROTO_UDP: {
    struct udphdr *udp = data + sizeof(*eth) + sizeof(*ip);
    if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*udp) > data_end)
      goto DROP;
    pcn_log(ctx, LOG_TRACE, "Packet is UDP: src_port %P, dst_port %P",
            udp->source, udp->dest);
    srcPort = udp->source;
    dstPort = udp->dest;
    break;
  }
  case IPPROTO_ICMP: {
    struct icmphdr *icmp = data + sizeof(*eth) + sizeof(*ip);
    if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*icmp) > data_end)
      goto DROP;
    pcn_log(ctx, LOG_TRACE, "Packet is ICMP: type %d, id %d", icmp->type,
            icmp->un.echo.id);

    // Consider the ICMP ID as a "port" number for easier handling
    srcPort = icmp->un.echo.id;
    dstPort = icmp->un.echo.id;
    break;
  }
  default:
    pcn_log(ctx, LOG_TRACE, "Unknown L4 proto %d, dropping", ip->protocol);
    goto DROP;
  }

  // Packet parsed, start session table lookup
  struct st_k key = {0, 0, 0, 0, 0};
  key.src_ip = srcIp;
  key.dst_ip = dstIp;
  key.src_port = srcPort;
  key.dst_port = dstPort;
  key.proto = proto;
  struct st_v *value;

  if (md->in_port == INTERNAL_PORT) {
    // Packet is inside -> outside, check egress session table
    value = egress_session_table.lookup(&key);
    if (value != NULL) {
      // Session table hit
      pcn_log(ctx, LOG_TRACE, "Egress session table: hit");

      newIp = value->new_ip;
      newPort = value->new_port;
      rule_type = NAT_SRC;

      update_session_table = 0;

      goto apply_nat;
    }
    pcn_log(ctx, LOG_TRACE, "Egress session table: miss");
  } else {
    // Packet is outside -> inside, check ingress session table
    value = ingress_session_table.lookup(&key);
    if (value != NULL) {
      // Session table hit
      pcn_log(ctx, LOG_TRACE, "Ingress session table: hit");

      newIp = value->new_ip;
      newPort = value->new_port;
      rule_type = NAT_DST;

      update_session_table = 0;

      goto apply_nat;
    }
    pcn_log(ctx, LOG_TRACE, "Ingress session table: miss");
  }

  // Session table miss, start rule lookup
  if (md->in_port == INTERNAL_PORT) {
    // Packet is inside -> outside, check SNAT/MASQUERADE rule table
    struct sm_k key = {0, 0};
    key.internal_netmask_len = 32;
    key.internal_ip = srcIp;
    struct sm_v *value = sm_rules.lookup(&key);
    if (value != NULL) {
      pcn_log(ctx, LOG_TRACE, "Egress rule table: hit");

      newIp = value->external_ip;
      newPort = get_free_port();
      rule_type = value->entry_type;

      goto apply_nat;
    }
    pcn_log(ctx, LOG_TRACE, "Egress rule table: miss");
  } else {
    // Packet is outside -> inside, check DNAT/PORTFORWARDING rule table
    struct dp_k key = {0, 0, 0};
    key.mask = 56;  // 32 (IP) + 16 (Port) + 8 (Proto)
    key.external_ip = dstIp;
    key.external_port = dstPort;
    key.proto = proto;
    struct dp_v *value = dp_rules.lookup(&key);
    if (value != NULL) {
      pcn_log(ctx, LOG_TRACE, "Ingress rule table: hit");

      newIp = value->internal_ip;
      newPort = value->internal_port;
      rule_type = value->entry_type;
      if (newPort == 0) {
        // Matching rule is DNAT, keep the same port number
        newPort = dstPort;
      }

      goto apply_nat;
    }
    pcn_log(ctx, LOG_TRACE, "Ingress rule table: miss");
  }

  // No matching entry was found in the session tables
  // No matching rule was found in the rule tables
  // -> Forward packet as it is
  goto proceed;

apply_nat:;
  if (update_session_table == 1) {
    // No session table exist for the packet, but a rule matched
    // -> Update the session tables in both directions

    struct st_k forward_key = {0, 0, 0, 0, 0};
    struct st_v forward_value = {0, 0};

    struct st_k reverse_key = {0, 0, 0, 0, 0};
    struct st_v reverse_value = {0, 0};

    if (rule_type == NAT_SRC || rule_type == NAT_MSQ) {
      // A rule matched in the inside -> outside direction

      // Session table entry for the outgoing packets
      forward_key.src_ip = srcIp;
      forward_key.dst_ip = dstIp;
      forward_key.src_port = srcPort;
      forward_key.dst_port = dstPort;
      forward_key.proto = proto;

      forward_value.new_ip = newIp;
      forward_value.new_port = newPort;
      forward_value.originating_rule_type = rule_type;

      // Session table entry for the incoming packets
      reverse_key.src_ip = dstIp;
      reverse_key.dst_ip = newIp;
      if (proto == IPPROTO_ICMP) {
        // For ICMP session table entries "source port" and "destination port"
        // must be the same, equal to the ICMP ID
        reverse_key.src_port = newPort;
      } else {
        reverse_key.src_port = dstPort;
      }
      reverse_key.dst_port = newPort;
      reverse_key.proto = proto;

      reverse_value.new_ip = srcIp;
      reverse_value.new_port = srcPort;
      reverse_value.originating_rule_type = rule_type;

      pcn_log(ctx, LOG_TRACE, "Updating session tables after SNAT");
      pcn_log(ctx, LOG_DEBUG, "New outgoing connection: %I:%P -> %I:%P", srcIp,
              srcPort, dstIp, dstPort);
    } else {
      // A rule matched in the outside -> inside direction

      // Session table entry for the outgoing packets
      forward_key.src_ip = newIp;
      forward_key.dst_ip = srcIp;
      forward_key.src_port = newPort;
      if (proto == IPPROTO_ICMP) {
        // For ICMP session table entries "source port" and "destination port"
        // must be the same, equal to the ICMP ID
        forward_key.dst_port = newPort;
      } else {
        forward_key.dst_port = srcPort;
      }
      forward_key.proto = proto;

      forward_value.new_ip = dstIp;
      forward_value.new_port = dstPort;
      forward_value.originating_rule_type = rule_type;

      // Session table entry for the incoming packets
      reverse_key.src_ip = srcIp;
      reverse_key.dst_ip = dstIp;
      reverse_key.src_port = srcPort;
      reverse_key.dst_port = dstPort;
      reverse_key.proto = proto;

      reverse_value.new_ip = newIp;
      reverse_value.new_port = newPort;
      reverse_value.originating_rule_type = rule_type;

      pcn_log(ctx, LOG_TRACE, "Updating session tables after DNAT");
      pcn_log(ctx, LOG_DEBUG, "New incoming connection: %I:%P -> %I:%P", srcIp,
              srcPort, dstIp, dstPort);
    }

    egress_session_table.update(&forward_key, &forward_value);
    ingress_session_table.update(&reverse_key, &reverse_value);

    pcn_log(ctx, LOG_TRACE,
            "Using ingress key: srcIp %I, dstIp %I, srcPort %P, dstPort %P",
            reverse_key.src_ip, reverse_key.dst_ip, reverse_key.src_port,
            reverse_key.dst_port);
    pcn_log(ctx, LOG_TRACE,
            "Using egress key: srcIp %I, dstIp %I, srcPort %P, dstPort %P",
            forward_key.src_ip, forward_key.dst_ip, forward_key.src_port,
            forward_key.dst_port);
  }

  // Modify packet
  uint32_t old_ip =
      (rule_type == NAT_SRC || rule_type == NAT_MSQ) ? srcIp : dstIp;
  uint32_t old_port =
      (rule_type == NAT_SRC || rule_type == NAT_MSQ) ? srcPort : dstPort;
  uint32_t new_ip = newIp;
  uint32_t new_port = newPort;

  uint32_t l3sum = pcn_csum_diff(&old_ip, 4, &new_ip, 4, 0);
  uint32_t l4sum = pcn_csum_diff(&old_port, 4, &new_port, 4, 0);

  switch (proto) {
  case IPPROTO_TCP: {
    struct tcphdr *tcp = data + sizeof(*eth) + sizeof(*ip);
    if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*tcp) > data_end)
      goto DROP;

    if (rule_type == NAT_SRC || rule_type == NAT_MSQ) {
      ip->saddr = new_ip;
      tcp->source = (__be16)new_port;
      pcn_log(ctx, LOG_TRACE, "Natted TCP packet: source, %I:%P -> %I:%P",
              old_ip, old_port, new_ip, new_port);
    } else {
      ip->daddr = new_ip;
      tcp->dest = (__be16)new_port;
      pcn_log(ctx, LOG_TRACE, "Natted TCP packet: destination, %I:%P -> %I:%P",
              old_ip, old_port, new_ip, new_port);
    }

    // Update checksums
    pcn_l4_csum_replace(ctx, TCP_CSUM_OFFSET, 0, l3sum, IS_PSEUDO | 0);
    pcn_l4_csum_replace(ctx, TCP_CSUM_OFFSET, 0, l4sum, 0);
    pcn_l3_csum_replace(ctx, IP_CSUM_OFFSET, 0, l3sum, 0);

    goto proceed;
  }
  case IPPROTO_UDP: {
    struct udphdr *udp = data + sizeof(*eth) + sizeof(*ip);
    if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*udp) > data_end)
      goto DROP;

    if (rule_type == NAT_SRC || rule_type == NAT_MSQ) {
      ip->saddr = new_ip;
      udp->source = (__be16)new_port;
      pcn_log(ctx, LOG_TRACE, "Natted UDP packet: source, %I:%P -> %I:%P",
              old_ip, old_port, new_ip, new_port);
    } else {
      ip->daddr = new_ip;
      udp->dest = (__be16)new_port;
      pcn_log(ctx, LOG_TRACE, "Natted UDP packet: destination, %I:%P -> %I:%P",
              old_ip, old_port, new_ip, new_port);
    }

    // Update checksums
    pcn_l4_csum_replace(ctx, UDP_CSUM_OFFSET, 0, l3sum, IS_PSEUDO | 0);
    pcn_l4_csum_replace(ctx, UDP_CSUM_OFFSET, 0, l4sum, 0);
    pcn_l3_csum_replace(ctx, IP_CSUM_OFFSET, 0, l3sum, 0);

    goto proceed;
  }
  case IPPROTO_ICMP: {
    struct icmphdr *icmp = data + sizeof(*eth) + sizeof(*ip);
    if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*icmp) > data_end)
      goto DROP;

    if (rule_type == NAT_SRC || rule_type == NAT_MSQ) {
      ip->saddr = new_ip;
      icmp->un.echo.id = (__be16)new_port;
      pcn_log(ctx, LOG_TRACE, "Natted ICMP packet: source, %I:%P -> %I:%P",
              old_ip, old_port, new_ip, new_port);
    } else {
      ip->daddr = new_ip;
      icmp->un.echo.id = (__be16)new_port;
      pcn_log(ctx, LOG_TRACE, "Natted ICMP packet: destination, %I:%P -> %I:%P",
              old_ip, old_port, new_ip, new_port);
    }

    // Update checksums
    pcn_l4_csum_replace(ctx, ICMP_CSUM_OFFSET, 0, l4sum, 0);
    pcn_l3_csum_replace(ctx, IP_CSUM_OFFSET, 0, l3sum, 0);

    goto proceed;
  }
  }

proceed:;
  // Session tables have been updated (if needed)
  // The packet has been modified (if needed)
  // Nothing more to do, forward the packet
  if (md->in_port == INTERNAL_PORT) {
    return pcn_pkt_redirect(ctx, md, EXTERNAL_PORT);
  } else {
    return pcn_pkt_redirect(ctx, md, INTERNAL_PORT);
  }
DROP:;
  pcn_log(ctx, LOG_INFO, "Dropping packet");
  return RX_DROP;
}
