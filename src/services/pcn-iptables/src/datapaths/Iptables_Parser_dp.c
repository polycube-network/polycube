/*
 * Copyright 2017 The Polycube Authors
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

/* =======================
   Parse packet
   ======================= */

#include <uapi/linux/ip.h>
#include <uapi/linux/udp.h>

#define IPPROTO_TCP 6
#define IPPROTO_UDP 17
#define IPPROTO_ICMP 1

#define ICMP_ECHOREPLY 0 /* Echo Reply			*/
#define ICMP_ECHO 8      /* Echo Request			*/

// packet metadata
struct packetHeaders {
  uint32_t srcIp;
  uint32_t dstIp;
  uint8_t l4proto;
  uint16_t srcPort;
  uint16_t dstPort;
  uint8_t flags;
  uint32_t seqN;
  uint32_t ackN;
  uint8_t connStatus;
  uint32_t sessionId;
  uint8_t direction;

  // conntrackCommit attributes
  uint8_t mask;
  uint8_t setMask;
  uint8_t clearMask;
  uint8_t state;
  uint32_t sequence;
  uint64_t ttl;
} __attribute__((packed));

// Following macro INGRESS/EGRESS Logic, are used in order to distinguish
// Parser for input/forward chain (declaring maps)
// And Parser for output chain

// This is the percpu array containing the current packet headers
#if _INGRESS_LOGIC
BPF_TABLE_SHARED("percpu_array", int, struct packetHeaders, packet, 1);

BPF_TABLE_SHARED("percpu_array", int, u64, pkts_default_Input, 1);
BPF_TABLE_SHARED("percpu_array", int, u64, bytes_default_Input, 1);

BPF_TABLE_SHARED("percpu_array", int, u64, pkts_default_Forward, 1);
BPF_TABLE_SHARED("percpu_array", int, u64, bytes_default_Forward, 1);
#endif

#if _EGRESS_LOGIC
BPF_TABLE("extern", int, struct packetHeaders, packet, 1);

BPF_TABLE_SHARED("percpu_array", int, u64, pkts_default_Output, 1);
BPF_TABLE_SHARED("percpu_array", int, u64, bytes_default_Output, 1);
#endif

struct elements {
  uint64_t bits[_MAXRULES];
};

// This is the percpu array containing the current bitvector
#if _INGRESS_LOGIC
BPF_TABLE_SHARED("percpu_array", int, struct elements, sharedEle, 1);
#endif

#if _EGRESS_LOGIC
BPF_TABLE("extern", int, struct elements, sharedEle, 1);
#endif

struct eth_hdr {
  __be64 dst : 48;
  __be64 src : 48;
  __be16 proto;
} __attribute__((packed));

/*The struct defined in tcp.h lets flags be accessed only one by one,
*it is not needed here.*/
struct tcp_hdr {
  __be16 source;
  __be16 dest;
  __be32 seq;
  __be32 ack_seq;
  __u8 res1 : 4, doff : 4;
  __u8 flags;
  __be16 window;
  __sum16 check;
  __be16 urg_ptr;
} __attribute__((packed));

struct icmphdr {
  u_int8_t type; /* message type */
  u_int8_t code; /* type sub-code */
  u_int16_t checksum;
  union {
    struct {
      u_int16_t id;
      u_int16_t sequence;
    } echo;            /* echo datagram */
    u_int32_t gateway; /* gateway address */
    struct {
      u_int16_t __unused;
      u_int16_t mtu;
    } frag; /* path mtu discovery */
  } un;
};

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  pcn_log(ctx, LOG_TRACE, "[_HOOK] [Parser] ");

  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;
  struct eth_hdr *ethernet = data;
  if (data + sizeof(*ethernet) > data_end)
    goto drop;
  if (ethernet->proto != bpf_htons(ETH_P_IP)) {
    /*Let everything that is not IP pass. */
    pcn_log(ctx, LOG_DEBUG,
            "[_HOOK] [Parser] [eht->proto: 0x%x] pkt not IP. Let this traffic "
            "pass.",
            ethernet->proto);
    return RX_OK;
  }

  struct iphdr *ip = NULL;
  ip = data + sizeof(struct eth_hdr);
  if (data + sizeof(struct eth_hdr) + sizeof(*ip) > data_end)
    goto drop;

  int key = 0;
  struct packetHeaders *pkt;

  pkt = packet.lookup(&key);
  if (pkt == NULL) {
    goto drop;
  }

  pkt->srcIp = ip->saddr;
  pkt->dstIp = ip->daddr;
  pkt->l4proto = ip->protocol;

  if (ip->protocol == IPPROTO_TCP) {
    struct tcp_hdr *tcp = NULL;
    tcp = data + sizeof(struct eth_hdr) + sizeof(*ip);
    if (data + sizeof(struct eth_hdr) + sizeof(*ip) + sizeof(*tcp) > data_end)
      goto drop;
    pkt->srcPort = tcp->source;
    pkt->dstPort = tcp->dest;
    pkt->seqN = tcp->seq;
    pkt->ackN = tcp->ack_seq;
    pkt->flags = tcp->flags;
  } else if (ip->protocol == IPPROTO_UDP) {
    struct udphdr *udp = NULL;
    udp = data + sizeof(struct eth_hdr) + sizeof(*ip);
    if (data + sizeof(struct eth_hdr) + sizeof(*ip) + sizeof(*udp) > data_end)
      goto drop;
    pkt->srcPort = udp->source;
    pkt->dstPort = udp->dest;
  } else if (ip->protocol == IPPROTO_ICMP) {
    if (data + 34 + sizeof(struct icmphdr) > data_end)
      goto drop;
    struct icmphdr *icmp = data + 34;
    if (icmp->type == ICMP_ECHO || icmp->type == ICMP_ECHOREPLY) {
      pkt->srcPort = icmp->un.echo.id;
    } else {
      pkt->srcPort = 0;
    }
    pkt->dstPort = 0;
  }

  pcn_log(ctx, LOG_TRACE, "[_HOOK] [Parser] srcIp: %I dstIp: %I l4proto:0x%x ",
          pkt->srcIp, pkt->dstIp, pkt->l4proto);
  pcn_log(ctx, LOG_TRACE, "[_HOOK] [Parser] sPort: %P dPort: %P ", pkt->srcPort,
          pkt->dstPort);

#if _HORUS_ENABLED
  call_bpf_program(ctx, _HORUS);
#endif

  // or INGRESS or EGRESS
  call_bpf_program(ctx, _CHAINSELECTOR);

drop:;
  pcn_log(ctx, LOG_DEBUG, "[_HOOK] [Parser] drop pkt ");
  return RX_DROP;
}
