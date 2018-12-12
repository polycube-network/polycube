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
#include <linux/jhash.h>

#define SESSIONS_TABLE_DIM 10000

#define CONFIG_TABLE_DIM 5

// load balancing algorithm, uncomment only one
#define LB_HASHING_SESSION  // hasing on (sip, dip, sport, dport, proto)
// #define LB_HASHING_SOURCE_IP // hashing on source ip

#if defined LB_HASHING_SESSION && defined LB_HASHING_SOURCE_IP
#error "define only one lb algorithm"
#endif

// frontent and backend ports are 1st and 2nd one
#define FRONTEND_PORT _FRONTEND_PORT
#define BACKEND_PORT _BACKEND_PORT

// frontend ip and mac addresses
#define FRONTEND_IP _FRONTEND_IP //0x6400000a       // 10.0.0.100
#define FRONTEND_MAC _FRONTEND_MAC //0xccbbaa010101  // 01:01:01:aa:bb:cc

// address used as mac_src in outgoing traffic
#define LB_MAC 0xddeeff020202
#define PRINT_MAC(x) (bpf_htonll(x) >> 16)

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

// sessions table key
struct sessions_key {
  __be32 ip_src;
  __be32 ip_dst;
  __be16 port_src;
  __be16 port_dst;
  __u8 proto;
};

// sessions table value
struct sessions_value {
  __be64 mac;
};

struct eth_hdr {
  __be64 dst : 48;
  __be64 src : 48;
  __be16 proto;
} __attribute__((packed));

// sessions table contains already existing sessions.
BPF_TABLE("lru_hash", struct sessions_key, struct sessions_value,
          sessions_table, SESSIONS_TABLE_DIM);

// todo it should be a percpu_array, right now map is not populated corretly by
// control plane
// config table contains the list of backend server of the pool, and the number
// of backend servers.
// config_table[0] = #servers (e.g. 2)
// config_table[1] = mac_address server1 (e.g. 01:01:01:ab:cd:ef)
// config_table[2] = mac_address server2 (e.g. 02:02:02:ab:cd:ef)

BPF_TABLE("array", u32, __be64, config_table, CONFIG_TABLE_DIM);

// implements arp responder on frontend interface
static __always_inline
int arp_responder(struct CTXTYPE *ctx, struct pkt_metadata *md,
                  struct eth_hdr *eth, struct arp_hdr *arp) {
  __be32 target_ip = arp->ar_tip;
  __be32 sender = 0;
  if (target_ip == FRONTEND_IP)
    sender = FRONTEND_IP;
  else
    return RX_DROP;

  pcn_log(ctx, LOG_DEBUG, "Somebody is asking for my address\n");

  // build arp response stating from arp request.
  // use FRONTEND_MAC and FRONTEND_IP
  __be64 remotemac = arp->ar_sha;
  __be32 remoteip = arp->ar_sip;
  arp->ar_op = bpf_htons(ARPOP_REPLY);
  arp->ar_tha = remotemac;
  arp->ar_sha = FRONTEND_MAC;
  arp->ar_sip = sender;
  arp->ar_tip = remoteip;
  eth->dst = remotemac;
  eth->src = FRONTEND_MAC;

  // response is sent back on frontend port
  return pcn_pkt_redirect(ctx, md, FRONTEND_PORT);
}

// lookup for existing sessions
// hit: forward the packet
// miss: apply load balancing algorithm, assign session to a backend server,
// forward the packet.
static inline struct sessions_value *get_sessions_value(__be32 ip_src,
                                                        __be32 ip_dst,
                                                        __be16 port_src,
                                                        __be16 port_dst,
                                                        __u8 proto) {
  // lookup in the sessions_table
  struct sessions_key sessions_key = {};
  sessions_key.ip_src = ip_src;
  sessions_key.ip_dst = ip_dst;
  sessions_key.port_src = port_src;
  sessions_key.port_dst = port_dst;
  sessions_key.proto = proto;
  struct sessions_value *sessions_value_p = sessions_table.lookup(&sessions_key);
  if (!sessions_value_p) {

    // pcn_log(ctx, LOG_ERR, "miss session_table\n");

    // create rule for sessions
    struct sessions_value sessions_value = {};

// define load balancing algorithm
#ifdef LB_HASHING_SOURCE_IP
    u32 check = jhash((const void *)&ip_src, sizeof(__be32), JHASH_INITVAL);
#endif
#ifdef LB_HASHING_SESSION
    u32 check = jhash((const void *)&sessions_key, sizeof(struct sessions_key),
                      JHASH_INITVAL);
#endif

    // lookup number of backend servers
    // hint: we can re-inject the code each time number of backend servers
    // changes. we can save a lookup.
    u32 id = 0;
    __be64 *n_backend_servers_p = config_table.lookup(&id);
    if (!n_backend_servers_p) {
      return 0;
    }

    // select backend server index
    id = check % *n_backend_servers_p + 1;

    // lookup mac for backend server
    __u64 *mac = config_table.lookup(&id);
    if (!mac) {
      return 0;
    }
    sessions_value.mac = *mac;

    // pcn_log(ctx, LOG_TRACE, "+create new session+ (id = %d) (mac = %M)\n", id, *mac);

    // update sessions table
    sessions_table.lookup_or_init(&sessions_key, &sessions_value);
    sessions_value_p = &sessions_value;
  } else {

    // pcn_log(ctx, LOG_DEBUG, "hit session_table");

  }
  return sessions_value_p;
}

static __always_inline
int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;
  struct eth_hdr *eth = data;
  if (data + sizeof(*eth) > data_end)
    goto DROP;

  pcn_log(ctx, LOG_TRACE, "(in_port = %P) (proto = %x)\n", md->in_port, eth->proto);

  // allow only traffic from FRONTEND_PORT
  if (md->in_port != FRONTEND_PORT)
    goto DROP;
  switch (eth->proto) {
    case htons(ETH_P_IP):
      goto ip;
    case htons(ETH_P_ARP):
      goto arp;
    default:
      goto DROP;
  }

ip:;
  // allow only packets directed to the lb frontend mac
  if (eth->dst != FRONTEND_MAC) {
    goto DROP;
  }

  struct iphdr *ip = data + sizeof(*eth);
  if (data + sizeof(*eth) + sizeof(*ip) > data_end)
    goto DROP;
  switch (ip->protocol) {
    case IPPROTO_UDP:
      goto udp;
    case IPPROTO_TCP:
      goto tcp;
    case IPPROTO_ICMP:
      goto icmp;
    default:
      goto DROP;
  }

arp : {
  struct arp_hdr *arp = data + sizeof(*eth);
  if (data + sizeof(*eth) + sizeof(*arp) > data_end)
    goto DROP;
  if (arp->ar_op == bpf_htons(ARPOP_REQUEST)) {
    return arp_responder(ctx, md, eth, arp);
  }
  return RX_DROP;
}

udp : {
  struct udphdr *udp = data + sizeof(*eth) + sizeof(*ip);
  if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*udp) > data_end)
    return RX_DROP;

  pcn_log(ctx, LOG_TRACE, "UDP packet. source:%P dest:%P\n", bpf_ntohs(udp->source), bpf_ntohs(udp->dest));

  struct sessions_value *sessions_value_p = get_sessions_value(
      ip->saddr, ip->daddr, udp->source, udp->dest, ip->protocol);
  if (sessions_value_p) {
    eth->dst = sessions_value_p->mac;
    eth->src = LB_MAC;

    pcn_log(ctx, LOG_TRACE, "UDP packet, redirect to (mac = %M)\n", PRINT_MAC(eth->dst));

    return pcn_pkt_redirect(ctx, md, BACKEND_PORT);
  } else {
    goto DROP;
  }
}

tcp : {
  struct tcphdr *tcp = data + sizeof(*eth) + sizeof(*ip);
  if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*tcp) > data_end)
    return RX_DROP;

  pcn_log(ctx, LOG_TRACE, "TCP packet, source: %P dest: %P\n", bpf_ntohs(tcp->source), bpf_ntohs(tcp->dest));

  struct sessions_value *sessions_value_p = get_sessions_value(
      ip->saddr, ip->daddr, tcp->source, tcp->dest, ip->protocol);
  if (sessions_value_p) {
    eth->dst = sessions_value_p->mac;
    eth->src = LB_MAC;

    pcn_log(ctx, LOG_TRACE, "TCP packet, redirect to (mac = %M)\n", PRINT_MAC(eth->dst));

    return pcn_pkt_redirect(ctx, md, BACKEND_PORT);
  } else {
    goto DROP;
  }
}

icmp : {
  struct icmphdr *icmp = data + sizeof(*eth) + sizeof(*ip);
  if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*icmp) > data_end)
    return RX_DROP;

  pcn_log(ctx, LOG_TRACE, "ICMP packet type: %d code: %d\n", icmp->type, icmp->code);
  pcn_log(ctx, LOG_TRACE, "ICMP packet id: %d seq: %d\n", icmp->un.echo.id, icmp->un.echo.sequence);

  // Only manage ICMP Request and Reply
  if (!((icmp->type == ICMP_ECHO) || (icmp->type == ICMP_ECHOREPLY)))
    goto DROP;
  struct sessions_value *sessions_value_p =
      get_sessions_value(ip->saddr, ip->daddr, 0, 0, ip->protocol);
  if (sessions_value_p) {
    eth->dst = sessions_value_p->mac;
    eth->src = LB_MAC;

    pcn_log(ctx, LOG_TRACE, "ICMP packet, redirect to (mac = %M)\n", PRINT_MAC(eth->dst));

    return pcn_pkt_redirect(ctx, md, BACKEND_PORT);
  } else {
    goto DROP;
  }
}

DROP:;

  pcn_log(ctx, LOG_TRACE, "DROP packet (port = %P)\n", md->in_port);

  return RX_DROP;
}
