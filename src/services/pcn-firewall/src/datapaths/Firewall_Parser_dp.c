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

#define TRACE _TRACE

#include <uapi/linux/ip.h>
#include <uapi/linux/udp.h>

#define IPPROTO_TCP 6
#define IPPROTO_UDP 17

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
};

BPF_TABLE_SHARED("percpu_array", int, struct packetHeaders, packetIngress, 1);
BPF_TABLE_SHARED("percpu_array", int, struct packetHeaders, packetEgress, 1);

struct elements {
  uint64_t bits[_MAXRULES];
};
BPF_TABLE_SHARED("percpu_array", int, struct elements, sharedEleIngress, 1);
BPF_TABLE_SHARED("percpu_array", int, struct elements, sharedEleEgress, 1);

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

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  pcn_log(ctx, LOG_DEBUG, "Code Parse receiving packet.");

  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;
  struct eth_hdr *ethernet = data;
  if (data + sizeof(*ethernet) > data_end)
    return RX_DROP;
  if (ethernet->proto != bpf_htons(ETH_P_IP)) {
    /*Let everything that is not IP pass. */
    pcn_log(ctx, LOG_DEBUG, "Packet not IP. ");
    if (md->in_port == _INGRESSPORT) {
      pcn_pkt_redirect(ctx, md, _EGRESSPORT);
      return RX_REDIRECT;
    } else {
      pcn_pkt_redirect(ctx, md, _INGRESSPORT);
      return RX_REDIRECT;
    }
  }

  struct iphdr *ip = NULL;
  ip = data + sizeof(struct eth_hdr);
  if (data + sizeof(struct eth_hdr) + sizeof(*ip) > data_end)
    return RX_DROP;

  int key = 0;
  struct packetHeaders *pkt;
  if (md->in_port == _INGRESSPORT) {
    pkt = packetIngress.lookup(&key);
  } else {
    pkt = packetEgress.lookup(&key);
  }
  if (pkt == NULL) {
    // Not possible
    return RX_DROP;
  }

  pkt->srcIp = ip->saddr;
  pkt->dstIp = ip->daddr;
  pkt->l4proto = ip->protocol;

  if (ip->protocol == IPPROTO_TCP) {
    struct tcp_hdr *tcp = NULL;
    tcp = data + sizeof(struct eth_hdr) + sizeof(*ip);
    if (data + sizeof(struct eth_hdr) + sizeof(*ip) + sizeof(*tcp) > data_end)
      return RX_DROP;
    pkt->srcPort = tcp->source;
    pkt->dstPort = tcp->dest;
    pkt->seqN = tcp->seq;
    pkt->ackN = tcp->ack_seq;
    pkt->flags = tcp->flags;
  } else if (ip->protocol == IPPROTO_UDP) {
    struct udphdr *udp = NULL;
    udp = data + sizeof(struct eth_hdr) + sizeof(*ip);
    if (data + sizeof(struct eth_hdr) + sizeof(*ip) + sizeof(*udp) > data_end)
      return RX_DROP;
    pkt->srcPort = udp->source;
    pkt->dstPort = udp->dest;
  }

  key=0;
  struct elements *result;

  if (md->in_port == _INGRESSPORT){
    #if _NR_ELEMENTS_INGRESS > 0
    result = sharedEleIngress.lookup(&key);
    if(result == NULL){
      //Not possible
      return RX_DROP;
    }
    #if _NR_ELEMENTS_INGRESS == 1
          (result->bits)[0] = 0x7FFFFFFFFFFFFFFF;
    #else
    #pragma unroll
          for (int i = 0; i < _NR_ELEMENTS_INGRESS; ++i) {
            /*This is the first module, it initializes the percpu*/
            (result->bits)[i] = 0x7FFFFFFFFFFFFFFF;
          }

    #endif
    #endif
  }

  if (md->in_port == _EGRESSPORT){
    #if _NR_ELEMENTS_EGRESS > 0
    result = sharedEleEgress.lookup(&key);
    if(result == NULL){
      //Not possible
      return RX_DROP;
    }
    #if _NR_ELEMENTS_EGRESS == 1
          (result->bits)[0] = 0x7FFFFFFFFFFFFFFF;
    #else
    #pragma unroll
          for (int i = 0; i < _NR_ELEMENTS_EGRESS; ++i) {
            /*This is the first module, it initializes the percpu*/
            (result->bits)[i] = 0x7FFFFFFFFFFFFFFF;
          }

    #endif
    #endif
  }

  call_ingress_program(ctx, _NEXT_HOP);
  return RX_DROP;
}
