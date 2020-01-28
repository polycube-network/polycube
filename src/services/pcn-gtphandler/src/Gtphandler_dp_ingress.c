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


#include <bcc/helpers.h>

#include <uapi/linux/if_ether.h>
#include <uapi/linux/ip.h>
#include <uapi/linux/in.h>
#include <uapi/linux/udp.h>


// #define MAX_USER_EQUIPMENTS n

// Following constants are added to the program at user level once the cube
// is attached to an interface
// Default values are used to inject the program before attachment
#ifndef LOCAL_IP
#define LOCAL_IP 0
#endif
#ifndef LOCAL_MAC
#define LOCAL_MAC 0
#endif

#define GTP_PORT 2152


struct eth_hdr {
  __be64 dst : 48;
  __be64 src : 48;
  __be16 proto;
} __attribute__((packed));

struct gtp1_header {	/* According to 3GPP TS 29.060. */
	u8 flags;  
	u8 type;
	__be16 length;
	__be32 tid;
} __attribute__((packed));

struct user_equipment {
  __be32 tunnel_endpoint;
  __be32 teid;
};

BPF_TABLE_SHARED("hash", __be32, struct user_equipment, user_equipments, MAX_USER_EQUIPMENTS);


#define GTP_ENCAP_SIZE (sizeof(struct iphdr) +      \
                        sizeof(struct udphdr) +     \
                        sizeof(struct gtp1_header))


static __always_inline
int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;

  // Check if packet is GTP encapsulated, if not pass it to the next cube

  struct eth_hdr *eth = data;
  if (data + sizeof(*eth) > data_end) {
    return RX_DROP;
  }

  if (eth->dst != LOCAL_MAC) {
    return RX_OK;
  }

  if (eth->proto != htons(ETH_P_IP)) {
    return RX_OK;
  }

  struct iphdr *ip = data + sizeof(*eth);
  if ((void *)ip + sizeof(*ip) > data_end) {
    return RX_DROP;
  }

  if (ip->daddr != LOCAL_IP) {
    return RX_OK;
  }

  if (ip->protocol != IPPROTO_UDP) {
    return RX_OK;
  }

  struct udphdr *udp = (void *)ip + 4*ip->ihl;
  if ((void *)udp + sizeof(*udp) > data_end) {
    return RX_DROP;
  }
 
  if (udp->dest != htons(GTP_PORT)) {
    return RX_OK;
  }

  struct gtp1_header *gtp = (void *)udp + sizeof(*udp);
  if ((void *)gtp + sizeof(*gtp) > data_end) {
    return RX_DROP;
  }

  // Save tunnel as traffic class
  md->traffic_class = ntohl(gtp->tid);

  // Remove encapsulation
#ifdef POLYCUBE_XDP
  // Move eth header forward
  struct eth_hdr *new_eth = data + GTP_ENCAP_SIZE;
  if ((void *)new_eth + sizeof(*eth) > data_end) {
    return RX_DROP;
  }
  memmove(new_eth, eth, sizeof(*eth));
  bpf_xdp_adjust_head(ctx, GTP_ENCAP_SIZE);

#else
  bpf_skb_adjust_room(ctx, (int32_t)-GTP_ENCAP_SIZE, BPF_ADJ_ROOM_MAC, 0);
#endif

  return RX_OK;
}