/*
 * Copyright 2020 The Polycube Authors
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


// Template parameters:
// DIRECTION       direction (ingress/egress) of the program
// SUBVECTS_COUNT  number of 64 bits elements composing the bitvector
// CLASSES_COUNT   number of classes currently configured (i.e. number of bits
//                 in the bitvector)
// MATCHING_TABLES declarations of matching tables
// MATCHING_CODE   code performing matchings on packet fields
// PARSE_L3        whether l3 header parsing is enabled
// PARSE_L4        whether l4 header parsing is enabled


#include <bcc/helpers.h>

#include <uapi/linux/if_ether.h>
#include <uapi/linux/ip.h>
#include <uapi/linux/in.h>
#include <uapi/linux/udp.h>
#include <uapi/linux/tcp.h>


struct eth_hdr {
  __be64 dst : 48;
  __be64 src : 48;
  __be16 proto;
} __attribute__((packed));

struct pkt_headers {
  __be64 smac;
  __be64 dmac;
  __be16 ethtype;
  __be32 srcip;
  __be32 dstip;
      u8 l4proto;
  __be16 sport;
  __be16 dport;
};

struct bitvector {
  u64 bits[_SUBVECTS_COUNT];
};

// Packet bitvector is stored into a percpu array to avoid exceeding the maximum
// stack size of 512 bytes
BPF_PERCPU_ARRAY(pkt_bitvector, struct bitvector, 1);

BPF_ARRAY(class_ids, u32, _CLASSES_COUNT);

BPF_TABLE("extern", int, u16, index64, 64);

_MATCHING_TABLES


static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  pcn_log(ctx, LOG_TRACE, "_DIRECTION parser: processing packet");

  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;

  struct eth_hdr *eth = data;
  if ((void *)(eth + 1) > data_end) {
    return RX_DROP;
  }

  struct pkt_headers pkt = {0};

  pkt.smac = eth->src;
  pkt.dmac = eth->dst;
  pkt.ethtype = eth->proto;

#if _PARSE_L3 || _PARSE_L4
  if (eth->proto != htons(ETH_P_IP)) {
    goto CONTINUE;
  }

  struct iphdr *ip = data + sizeof(*eth);
  if ((void *)(ip + 1) > data_end) {
    return RX_DROP;
  }

  pkt.srcip = ip->saddr;
  pkt.dstip = ip->daddr;
  pkt.l4proto = ip->protocol;

#if _PARSE_L4
  if (ip->protocol == IPPROTO_TCP) {
    struct tcphdr *tcp = (void *)ip + 4*ip->ihl;
    if ((void *)(tcp + 1) > data_end) {
        return RX_DROP;
    }

    pkt.sport = tcp->source;
    pkt.dport = tcp->dest;

  } else if (ip->protocol == IPPROTO_UDP) {
    struct udphdr *udp = (void *)ip + 4*ip->ihl;
    if ((void *)(udp + 1) > data_end) {
        return RX_DROP;
    }
    
    pkt.sport = udp->source;
    pkt.dport = udp->dest;
  }

#endif  // _PARSE_L4
#endif  // _PARSE_L3 || _PARSE_L4

CONTINUE:;
  int zero = 0;
  struct bitvector *pkt_bv = pkt_bitvector.lookup(&zero);
  if (!pkt_bv) {
    return RX_DROP;
  }

  // Initialize packet bitvector
  for (int i = 0; i < _SUBVECTS_COUNT; i++) {
    pkt_bv->bits[i] = 0xffffffffffffffff;
  }


  _MATCHING_CODE


  int matching_class_index = -1;
  u16 *matching_res;
  for (int i = 0; i < _SUBVECTS_COUNT; i++) {
    u64 bits = pkt_bv->bits[i];
    if (bits != 0) {
      int index = (int)(((bits ^ (bits - 1)) * 0x03f79d71b4cb0a89) >> 58);

      matching_res = index64.lookup(&index);
      if (!matching_res) {
        return RX_DROP;
      }

      matching_class_index = *matching_res + i * 64;

      break;
    }
  }

  if (matching_class_index >= 0) {
    u32 *id = class_ids.lookup(&matching_class_index);
    if (!id) {
      return RX_DROP;
    }

    md->traffic_class = *id;

    pcn_log(ctx, LOG_TRACE, "_DIRECTION tagger: packet tagged with class %d",
            *id);
  
  } else  {
    pcn_log(ctx, LOG_TRACE, "_DIRECTION tagger: no matching class found");
  }

  return RX_OK;
}