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

#include <bcc/helpers.h>
#include <uapi/linux/if_ether.h>
#include <uapi/linux/in.h>
#include <uapi/linux/ip.h>
#include <uapi/linux/tcp.h>
#include <uapi/linux/udp.h>

struct eth_hdr {
  __be64 dst : 48;
  __be64 src : 48;
  __be16 proto;
} __attribute__((packed));

struct packetHeaders {
  uint64_t srcMac;
  uint64_t dstMac;
  uint16_t vlan;
  bool vlan_present;
  bool ip;
  uint32_t srcIp;
  uint32_t dstIp;
  uint8_t l4proto;
  uint16_t srcPort;
  uint16_t dstPort;
};

BPF_TABLE_SHARED("percpu_array", int, struct packetHeaders, packet, 1);

static __always_inline int handle_rx(struct CTXTYPE *ctx,
                                     struct pkt_metadata *md) {
  pcn_log(ctx, LOG_TRACE, "Parser receiving packet from port %d", md->in_port);

  int key = 0;
  struct packetHeaders *pkt;
  pkt = packet.lookup(&key);
  if (pkt == NULL) {
    return RX_DROP;
  }
  /* Parsing L2 */
  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;
  struct eth_hdr *ethernet = data;
  if (data + sizeof(*ethernet) > data_end)
    return RX_DROP;
  pkt->srcMac = ethernet->src;
  pkt->dstMac = ethernet->dst;
  uint16_t ether_type = ethernet->proto;

#ifdef POLYCUBE_XDP
  pkt->vlan_present = pcn_is_vlan_present(ctx);
  if (pkt->vlan_present) {
    if (pcn_get_vlan_id(ctx, &(pkt->vlan), &ether_type) < 0)
      return RX_DROP;
  }
#else
  pkt->vlan_present = ctx->vlan_present;
  if (pkt->vlan_present) {
    ether_type = ctx->vlan_proto;
    pkt->vlan = (uint16_t)(ctx->vlan_tci & 0x0fff);
  }
#endif

  /* Parsing L3 */
  struct iphdr *ip = NULL;
  struct tcphdr *tcp = NULL;
  struct udphdr *udp = NULL;
  pkt->ip = 0;
#if LEVEL > 2
  if (ether_type == bpf_htons(ETH_P_IP)) {
    ip = data + sizeof(*ethernet);
    if (data + sizeof(*ethernet) + sizeof(*ip) > data_end)
      return RX_DROP;
    pkt->ip = 1;
    pkt->srcIp = ip->saddr;
    pkt->dstIp = ip->daddr;
#if LEVEL == 4
    uint8_t header_len = 4 * ip->ihl;
    if (ip->protocol == IPPROTO_TCP) {
      tcp = data + sizeof(*ethernet) + header_len;
      if (data + sizeof(*ethernet) + header_len + sizeof(*tcp) > data_end)
        return RX_DROP;
      pkt->l4proto = IPPROTO_TCP;
      pkt->srcPort = tcp->source;
      pkt->dstPort = tcp->dest;
    } else if (ip->protocol == IPPROTO_UDP) {
      uint8_t header_len = 4 * ip->ihl;       //we have to redefine this to avoid errors
      udp = data + sizeof(*ethernet) + header_len;
      if (data + sizeof(*ethernet) + header_len + sizeof(*udp) > data_end)
        return RX_DROP;
      pkt->l4proto = IPPROTO_UDP;
      pkt->srcPort = udp->source;
      pkt->dstPort = udp->dest;
    }
#endif
  }
#endif
  call_ingress_program(ctx, 1);
  return RX_DROP;
}