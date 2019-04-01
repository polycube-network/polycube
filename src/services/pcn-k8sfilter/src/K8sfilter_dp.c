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

#include <linux/jhash.h>
#include <uapi/linux/if_arp.h>
#include <uapi/linux/ip.h>
#include <uapi/linux/tcp.h>
#include <uapi/linux/udp.h>

#ifndef EXTERNAL_PORT
#define EXTERNAL_PORT 0
#endif

#ifndef INTERNAL_PORT
#define INTERNAL_PORT 0
#endif

#ifndef NODEPORT_RANGE_LOW
#define NODEPORT_RANGE_LOW (30000)
#endif

#ifndef NODEPORT_RANGE_HIGH
#define NODEPORT_RANGE_HIGH (32767)
#endif

struct eth_hdr {
  __be64 dst : 48;
  __be64 src : 48;
  __be16 proto;
} __attribute__((packed));

static __always_inline int handle_rx(struct CTXTYPE *ctx,
                                     struct pkt_metadata *md) {
  pcn_log(ctx, LOG_TRACE, "k8s received packet");

  if (md->in_port == INTERNAL_PORT)
    return pcn_pkt_redirect(ctx, md, EXTERNAL_PORT);

  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;

  struct eth_hdr *eth = data;
  if (data + sizeof(*eth) > data_end)
    return RX_OK;

  if (eth->proto != htons(ETH_P_IP))
    return RX_OK;

  struct iphdr *ip = data + sizeof(*eth);
  if (data + sizeof(*eth) + sizeof(*ip) > data_end)
    return RX_OK;

  __be16 dst_port;

  if (ip->protocol == IPPROTO_UDP) {
    struct udphdr *udp = data + sizeof(*eth) + sizeof(*ip);
    if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*udp) > data_end)
      return RX_OK;
    dst_port = udp->dest;
  } else if (ip->protocol == IPPROTO_TCP) {
    struct tcphdr *tcp = data + sizeof(*eth) + sizeof(*ip);
    if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*tcp) > data_end)
      return RX_OK;
    dst_port = tcp->dest;
  } else {
    return RX_OK;
  }

  if (ntohs(dst_port) >= NODEPORT_RANGE_LOW &&
      ntohs(dst_port) <= NODEPORT_RANGE_HIGH) {
    pcn_log(ctx, LOG_DEBUG, "Sending packet to internal port");
    return pcn_pkt_redirect(ctx, md, INTERNAL_PORT);
  }

  pcn_log(ctx, LOG_TRACE, "Sending packet to stack");
  return RX_OK;
}
