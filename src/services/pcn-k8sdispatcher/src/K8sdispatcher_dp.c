/*
 * Copyright 2022 The Polycube Authors
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

#ifndef FRONTEND_PORT
#define FRONTEND_PORT 0
#endif

#ifndef BACKEND_PORT
#define BACKEND_PORT 0
#endif

#ifndef NODE_IP
#define NODE_IP 0
#endif

#ifndef INTERNAL_SRC_IP
#define INTERNAL_SRC_IP 0
#endif

#define SESSION_MAP_DIM 32768

enum OPERATION_TYPE { OP_XLATE_SRC = 0, OP_XLATE_DST = 1 };

enum ORIGINATING_RULE_TYPE { RULE_POD_TO_EXT = 0, RULE_NODEPORT_CLUSTER = 1 };

enum EXTERNAL_TRAFFIC_POLICY_TYPE { ETP_LOCAL = 0, ETP_CLUSTER = 1 };

enum SLOWPATH_REASON { REASON_ARP_REPLY = 0 };

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

struct arp_hdr {
  __be16 ar_hrd;        /* format of hardware address	*/
  __be16 ar_pro;        /* format of protocol address	*/
  unsigned char ar_hln; /* length of hardware address	*/
  unsigned char ar_pln; /* length of protocol address	*/
  __be16 ar_op;         /* ARP opcode (command)		*/
  __be64 ar_sha : 48;   /* sender hardware address	*/
  __be32 ar_sip;        /* sender IP address		*/
  __be64 ar_tha : 48;   /* target hardware address	*/
  __be32 ar_tip;        /* target IP address		*/
} __attribute__((packed));

// Session table
struct st_k {
  uint32_t src_ip;
  uint32_t dst_ip;
  uint16_t src_port;
  uint16_t dst_port;
  uint8_t proto;
} __attribute__((packed));
struct st_v {
  uint32_t new_ip;
  uint16_t new_port;
  uint8_t operation;
  uint8_t originating_rule;
} __attribute__((packed));
BPF_TABLE("lru_hash", struct st_k, struct st_v, egress_session_table,
          SESSION_MAP_DIM);
BPF_TABLE("lru_hash", struct st_k, struct st_v, ingress_session_table,
          SESSION_MAP_DIM);

// NodePort rules table
struct nt_k {
  uint16_t port;
  uint8_t proto;
} __attribute__((packed));
;
struct nt_v {
  uint16_t external_traffic_policy;
} __attribute__((packed));
;

BPF_F_TABLE("hash", struct nt_k, struct nt_v, npr_table, 1024,
            BPF_F_NO_PREALLOC);

// Port numbers
struct free_port_entry {
  uint16_t free_port;
  struct bpf_spin_lock lock;
};

BPF_TABLE("array", uint32_t, struct free_port_entry, free_port_map, 1);

static inline __be16 get_free_port() {
  uint32_t key = 0;
  uint16_t free_port = 0;
  struct free_port_entry *entry = free_port_map.lookup(&key);
  if (!entry) {  // never happen for array
    return 0;
  }
  bpf_spin_lock(&entry->lock);
  if (entry->free_port < 1024 || entry->free_port == 65535) {
    entry->free_port = 1024;
  }
  free_port = entry->free_port;
  entry->free_port++;
  bpf_spin_unlock(&entry->lock);
  return bpf_htons(free_port);
}

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  // Disable data plane if initialization has not been completed yet
  if (FRONTEND_PORT == BACKEND_PORT) {
    return RX_DROP;
  }
  // NAT processing happens in 4 steps:
  // 1) packet parsing
  // 2) session table lookup
  // 3) rule lookup
  // 4) packet modification

  // 1) Parse packet
  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;

  struct eth_hdr *eth = data;
  if ((void *)(eth + 1) > data_end) {
    pcn_log(ctx, LOG_TRACE,
            "Dropped Ethernet pkt - (in_port: %d) (reason: inconsistent_size)",
            md->in_port);
    return RX_DROP;
  }

  switch (eth->proto) {
  case htons(ETH_P_IP):
    goto IP;  // ipv4 packet
  case htons(ETH_P_ARP): {
    goto ARP;  // ARP packet
  }
  default:
    if (md->in_port == BACKEND_PORT) {
      pcn_log(ctx, LOG_TRACE,
              "Received unsupported pkt from BACKEND port: sent to stack - "
              "(in_port: %d) (proto: 0x%x)",
              md->in_port, bpf_htons(eth->proto));
    } else {
      pcn_log(ctx, LOG_TRACE,
              "Received unsupported pkt from FRONTEND port: sent to stack - "
              "(in_port: %d) (proto: 0x%x)",
              md->in_port, bpf_htons(eth->proto));
    }
    return RX_OK;
  }

IP:;
  struct iphdr *ip = (void *)(eth + 1);
  if ((void *)(ip + 1) > data_end) {
    pcn_log(ctx, LOG_TRACE,
            "Dropped IP pkt - (in_port: %d) (reason: inconsistent_size)",
            md->in_port);
    return RX_DROP;
  }

  // check if ip dest is for this host
  if (md->in_port == FRONTEND_PORT && ip->daddr != NODE_IP) {
    pcn_log(ctx, LOG_TRACE,
            "Pkt coming from FRONTEND port is not for host: sent to stack - "
            "(in_port: %d) (dst_ip: %I)",
            md->in_port, ip->daddr);
    return RX_OK;
  }

  // Extract L3 packet data
  uint32_t src_ip = ip->saddr;
  uint32_t dst_ip = ip->daddr;
  uint8_t proto = ip->protocol;

  // Extract L4 segment data
  uint16_t src_port = 0, dst_port = 0;
  // The following pointers are used to update a TCP port, a UDP port or the
  // ICMP echo id in the same way regardless the l4 protocol; same for the
  // checksum offset
  uint16_t *src_port_ptr = NULL, *dst_port_ptr = NULL;
  int l4_csum_offset = 0;

  switch (ip->protocol) {
  case IPPROTO_TCP: {
    struct tcphdr *tcp = (void *)(ip + 1);
    if ((void *)(tcp + 1) > data_end) {
      pcn_log(ctx, LOG_TRACE,
              "Dropped TCP pkt - (in_port: %d) (reason: inconsistent_size)",
              md->in_port);
      return RX_DROP;
    }

    src_port = tcp->source;
    dst_port = tcp->dest;
    src_port_ptr = &tcp->source;
    dst_port_ptr = &tcp->dest;
    l4_csum_offset = TCP_CSUM_OFFSET;
    break;
  }
  case IPPROTO_UDP: {
    struct udphdr *udp = (void *)(ip + 1);
    if ((void *)(udp + 1) > data_end) {
      pcn_log(ctx, LOG_TRACE,
              "Dropped UDP pkt - (in_port: %d) (reason: inconsistent_size)",
              md->in_port);
      return RX_DROP;
    }

    src_port = udp->source;
    dst_port = udp->dest;
    src_port_ptr = &udp->source;
    dst_port_ptr = &udp->dest;
    l4_csum_offset = UDP_CSUM_OFFSET;
    break;
  }
  case IPPROTO_ICMP: {
    struct icmphdr *icmp = (void *)(ip + 1);
    if ((void *)(icmp + 1) > data_end) {
      pcn_log(ctx, LOG_TRACE,
              "Dropped ICMP pkt - (in_port: %d) (reason: inconsistent_size)",
              md->in_port);
      return RX_DROP;
    }

    // Consider the ICMP ID as a "port" number for easier handling
    src_port = icmp->un.echo.id;
    dst_port = icmp->un.echo.id;
    src_port_ptr = &icmp->un.echo.id;
    dst_port_ptr = &icmp->un.echo.id;
    l4_csum_offset = ICMP_CSUM_OFFSET;
    break;
  }
  default: {
    pcn_log(ctx, LOG_TRACE,
            "Dropped IP pkt - (in_port: %d) (reason: unsupported_type) "
            "(ip_proto: %d)",
            md->in_port, ip->protocol);
    return RX_DROP;
  }
  }

  // 2) Packet parsed, start session table lookup
  // NAT data
  uint32_t new_ip = 0;
  uint16_t new_port = 0;
  uint8_t operation = 0;
  struct st_k session_key = {.src_ip = src_ip,
                             .dst_ip = dst_ip,
                             .src_port = src_port,
                             .dst_port = dst_port,
                             .proto = proto};
  struct st_v *session_entry = (md->in_port == BACKEND_PORT)
                                   ? egress_session_table.lookup(&session_key)
                                   : ingress_session_table.lookup(&session_key);
  if (session_entry) {
    // Extract NAT data
    new_ip = session_entry->new_ip;
    new_port = session_entry->new_port;
    operation = session_entry->operation;
    goto APPLY_NAT;
  }

  uint8_t originating_rule = 0;

  // 3) Session table miss, start rule lookup
  if (md->in_port == BACKEND_PORT) {  // inside -> outside
    // Check egress rules

    // Rule: RESP_NP_LOCAL
    // Response for a NodePort Svc with externalTrafficPolicy=LOCAL => FORWARD
    if (src_ip == NODE_IP) {
      pcn_log(ctx, LOG_TRACE,
              "Matched egress rule RESP_NP_LOCAL: redirected pkt as is to "
              "FRONTEND port - (src: %I:%P, dst: %I:%P)",
              src_ip, src_port, dst_ip, dst_port);
      return pcn_pkt_redirect(ctx, md, FRONTEND_PORT);
    }

    // Rule: POD_TO_EXT
    // A Pod want to reach the external world => SNAT
    new_ip = NODE_IP;
    new_port = get_free_port();
    operation = OP_XLATE_SRC;
    originating_rule = RULE_POD_TO_EXT;
    pcn_log(ctx, LOG_TRACE,
            "Matched egress rule POD_TO_EXT: chosen SNAT - (src: %I:%P, dst: "
            "%I:%P)",
            src_ip, src_port, dst_ip, dst_port);
  } else {  // outside -> inside
    // Check ingress rules

    struct nt_k npr_key = {.port = dst_port, .proto = proto};
    struct nt_v *npr_value = npr_table.lookup(&npr_key);

    // Incoming packet doesn't match any NodePort rule => stack
    if (npr_value == NULL) {
      pcn_log(ctx, LOG_TRACE,
              "No ingress rule match: sent to stack - (src: %I:%P, dst: %I:%P)",
              src_ip, src_port, dst_ip, dst_port);
      return RX_OK;
    }

    // Rule: REQ_NP_LOCAL
    // Request for a NodePort Svc with externalTrafficPolicy=LOCAL => FORWARD
    if (npr_value->external_traffic_policy == ETP_LOCAL) {
      pcn_log(ctx, LOG_TRACE,
              "Matched ingress rule REQ_NP_LOCAL: redirected pkt as is to "
              "BACKEND port - (src: %I:%P, dst: %I:%P)",
              src_ip, src_port, dst_ip, dst_port);
      return pcn_pkt_redirect(ctx, md, BACKEND_PORT);
    }

    // Rule: REQ_NP_CLUSTER
    // Request for a NodePort Svc with externalTrafficPolicy=CLUSTER => SNAT
    new_ip = INTERNAL_SRC_IP;
    new_port = get_free_port();
    operation = OP_XLATE_SRC;
    originating_rule = RULE_NODEPORT_CLUSTER;

    pcn_log(
        ctx, LOG_TRACE,
        "Matched egress rule REQ_NP_CLUSTER: chosen SNAT - (src: %I:%P, dst: "
        "%I:%P)",
        src_ip, src_port, dst_ip, dst_port);
  }

  // No session table exist for the packet but a rule matched, so both sessions
  // tables must be updated. In the following, the forward table is intended
  // to be the table that handles the sessions in the same direction of the
  // actual packet; similarly, the reverse table is intended to be
  // the table that handles the sessions in the opposite direction of the
  // actual packet. Example:
  // packet coming from the BACKEND port
  // - forward table = egress session table
  // - reverse table = ingress session table

  // Regardless the type of NAT, the forward table key always match the original
  // packet quintuple data. Moreover, the forward table value always stores
  // the new ip, the new port and the same kind of operation that will be
  // applied on the actual packet. The originating rule is determined during
  // the rules scan
  struct st_k forward_key = {.src_ip = src_ip,
                             .dst_ip = dst_ip,
                             .src_port = src_port,
                             .dst_port = dst_port,
                             .proto = proto};
  struct st_v forward_value = {.new_ip = new_ip,
                               .new_port = new_port,
                               .operation = operation,
                               .originating_rule = originating_rule};

  // The reverse table key and value depend on the kind of operation that will
  // be applied on the actual packet
  struct st_k reverse_key = {0, 0, 0, 0, 0};
  struct st_v reverse_value = {0, 0};

  if (operation == OP_XLATE_SRC) {
    reverse_key.src_ip = dst_ip;
    reverse_key.dst_ip = new_ip;
    // During the NAT operation, the ICMP ID will be replaced with the value
    // of "new port": this means that for the reverse traffic, the session will
    // be identified by a packet having both "source port" and "destination port"
    // equal to "new port"
    reverse_key.src_port = (proto == IPPROTO_ICMP) ? new_port : dst_port;
    reverse_key.dst_port = new_port;

    reverse_value.new_ip = src_ip;
    reverse_value.new_port = src_port;
    reverse_value.operation = OP_XLATE_DST;
  } else {
    reverse_key.src_ip = new_ip;
    reverse_key.dst_ip = src_ip;
    reverse_key.src_port = new_port;
    // During the NAT operation, the ICMP ID will be replaced with the value
    // of "new port": this means that for the reverse traffic, the session will
    // be identified by a packet having both "source port" and "destination port"
    // equal to "new port"
    reverse_key.dst_port = (proto == IPPROTO_ICMP) ? new_port : src_port;

    reverse_value.new_ip = dst_ip;
    reverse_value.new_port = dst_port;
    reverse_value.operation = OP_XLATE_SRC;
  }
  // The protocol type in the reverse table key and the originating rule type
  // in the reverse table value do not depend on the operation applied on the
  // actual packet
  reverse_key.proto = proto;
  reverse_value.originating_rule = originating_rule;

  if (md->in_port == BACKEND_PORT) {
    egress_session_table.update(&forward_key, &forward_value);
    ingress_session_table.update(&reverse_key, &reverse_value);
  } else {
    ingress_session_table.update(&forward_key, &forward_value);
    egress_session_table.update(&reverse_key, &reverse_value);
  }

  pcn_log(ctx, LOG_TRACE, "Created new session - (src: %I:%P, dst: %I:%P)",
          src_ip, src_port, dst_ip, dst_port);

APPLY_NAT:;
  // Save old values in order to update checksums after the NAT is applied
  uint32_t old_ip;
  uint16_t old_port;
  if (operation == OP_XLATE_SRC) {
    old_ip = src_ip;
    old_port = src_port;
    ip->saddr = new_ip;
    *src_port_ptr = new_port;
    pcn_log(ctx, LOG_TRACE, "Applied SNAT - (src: %I:%P, new_src: %I:%P)",
            old_ip, old_port, new_ip, new_port);
  } else {
    old_ip = dst_ip;
    old_port = dst_port;
    ip->daddr = new_ip;
    *dst_port_ptr = new_port;
    pcn_log(ctx, LOG_TRACE, "Applied DNAT - (dst: %I:%P, new_dst: %I:%P)",
            old_ip, old_port, new_ip, new_port);
  }

  // Update checksums
  if (proto != IPPROTO_ICMP) {  // only for UDP and TCP
    pcn_l4_csum_replace(ctx, l4_csum_offset, old_ip, new_ip, IS_PSEUDO | 4);
  }
  pcn_l4_csum_replace(ctx, l4_csum_offset, old_port, new_port, 2);
  pcn_l3_csum_replace(ctx, IP_CSUM_OFFSET, old_ip, new_ip, 4);

  // Session tables have been updated (if needed)
  // The packet has been modified (if needed)
  // Nothing more to do, forward the packet
  return pcn_pkt_redirect(
      ctx, md, md->in_port == BACKEND_PORT ? FRONTEND_PORT : BACKEND_PORT);

ARP:;
  struct arp_hdr *arp = (void *)(eth + 1);
  if ((void *)(arp + 1) > data_end) {
    pcn_log(ctx, LOG_TRACE,
            "Dropped ARP pkt - (in_port: %d) (reason: inconsistent_size)",
            md->in_port);
    return RX_DROP;
  }

  if (md->in_port == BACKEND_PORT) {  // inside -> outside
    // If a packet coming from the backend port
    // - is an ARP request => it is sent to the frontend port
    // - otherwise it is dropped
    if (arp->ar_op == bpf_htons(ARPOP_REQUEST)) {
      pcn_log(ctx, LOG_TRACE,
              "Received ARP request pkt from BACKEND port: redirected to "
              "FRONTEND port - (in_port: %d) (out_port: %d)",
              md->in_port, FRONTEND_PORT);
      return pcn_pkt_redirect(ctx, md, FRONTEND_PORT);
    }
    pcn_log(ctx, LOG_TRACE,
            "Dropped non-request ARP pkt coming from BACKEND port - (in_port: "
            "%d) (arp_opcode: %d)",
            md->in_port, bpf_htons(arp->ar_op));
  } else {  // outside -> inside
    // If a packet coming from the frontend port
    // - is an ARP request => it is sent to the stack
    // - is an ARP reply => a copy is sent to the slowpath and the original one
    // to the stack
    // - otherwise it is dropped
    if (arp->ar_op == bpf_ntohs(ARPOP_REQUEST)) {
      pcn_log(ctx, LOG_TRACE,
              "Received ARP request pkt from FRONTEND port: sent to the stack "
              "- (in_port: %d)",
              md->in_port);
      return RX_OK;
    } else if (arp->ar_op == bpf_htons(ARPOP_REPLY)) {
      pcn_log(ctx, LOG_TRACE,
              "Received ARP reply pkt from FRONTEND port: sent a copy to "
              "slow path and a copy to the stack - (in_port: %d)",
              md->in_port);
      // a copy is sent to the backend port through the slowpath and a copy is
      // sent to the stack
      pcn_pkt_controller(ctx, md, REASON_ARP_REPLY);
      return RX_OK;
    }
    pcn_log(ctx, LOG_TRACE,
            "Dropped non-reply ARP pkt coming from FRONTEND port - (in_port: "
            "%d) (arp_opcode: %d)",
            md->in_port, bpf_htons(arp->ar_op));
  }
  return RX_DROP;
}