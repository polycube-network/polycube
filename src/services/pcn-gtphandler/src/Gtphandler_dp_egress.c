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
#define GTP_TYPE_GPDU 255  // User data packet (T-PDU) plus GTP-U header 
#define GTP_FLAGS 0x30     // Version: GTPv1, Protocol Type: GTP, Others: 0


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

BPF_TABLE("extern", __be32, struct user_equipment, user_equipments, MAX_USER_EQUIPMENTS);


#define IP_CSUM_OFFSET (sizeof(struct eth_hdr) + offsetof(struct iphdr, check))
#define GTP_ENCAP_SIZE (sizeof(struct iphdr) +      \
                        sizeof(struct udphdr) +     \
                        sizeof(struct gtp1_header))


static __always_inline
int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;

  struct eth_hdr *eth = data;
  if (data + sizeof(*eth) > data_end) {
    return RX_DROP;
  }

  if (eth->proto != htons(ETH_P_IP)) {
    return RX_OK;
  }

  struct iphdr *ip = data + sizeof(*eth);
  if ((void *)ip + sizeof(*ip) > data_end) {
    return RX_DROP;
  }

  // Check if packet is directed to a user equipment
  struct user_equipment *ue = user_equipments.lookup(&ip->daddr);
  if (!ue) {
    return RX_OK;
  }

  // Add space for encapsulation to packet buffer
#ifdef POLYCUBE_XDP
  bpf_xdp_adjust_head(ctx, (int32_t)-GTP_ENCAP_SIZE);
#else
  bpf_skb_adjust_room(ctx, GTP_ENCAP_SIZE, BPF_ADJ_ROOM_MAC, 0);
#endif

  // Packet buffer changed, all pointers need to be recomputed
  data = (void *)(long)ctx->data;
  data_end = (void *)(long)ctx->data_end;

  eth = data;
  if (data + sizeof(*eth) > data_end) {
    return RX_DROP;
  }

#ifdef POLYCUBE_XDP
  // Space allocated before packet buffer, move eth header
  struct eth_hdr *old_eth = data + GTP_ENCAP_SIZE;
  if ((void *)old_eth + sizeof(*eth) > data_end) {
    return RX_DROP;
  }
  memmove(eth, old_eth, sizeof(*eth));
#endif

  ip = data + sizeof(*eth);
  if ((void *)ip + sizeof(*ip) > data_end) {
    return RX_DROP;
  }

  struct iphdr *inner_ip = (void *)ip + GTP_ENCAP_SIZE;
  if ((void *)inner_ip + sizeof(*inner_ip) > data_end) {
    return RX_DROP;
  }

  // Add the outer IP header
  ip->version = 4;
  ip->ihl = 5;  // No options
  ip->tos = 0;
  ip->tot_len = htons(ntohs(inner_ip->tot_len) + GTP_ENCAP_SIZE);
  ip->id = 0;  // No fragmentation
  ip->frag_off = 0x0040;  // Don't fragment; Fragment offset = 0
  ip->ttl = 64;
  ip->protocol = IPPROTO_UDP;
  ip->check = 0;
  ip->saddr = LOCAL_IP;
  ip->daddr = ue->tunnel_endpoint;

  // Add the UDP header
  struct udphdr *udp = (void *)ip + sizeof(*ip);
  if ((void *)udp + sizeof(*udp) > data_end) {
    return RX_DROP;
  }
  udp->source = htons(GTP_PORT);
  udp->dest = htons(GTP_PORT);
  udp->len = htons(ntohs(inner_ip->tot_len) + sizeof(*udp) + sizeof(struct gtp1_header));
  udp->check = 0;

  // Add the GTP header
  struct gtp1_header *gtp = (void *)udp + sizeof(*udp);
  if ((void *)gtp + sizeof(*gtp) > data_end) {
    return RX_DROP;
  }
  gtp->flags = GTP_FLAGS;  
  gtp->type = GTP_TYPE_GPDU;
  gtp->length = inner_ip->tot_len;
  gtp->tid = ue->teid;

  // Compute l3 checksum
  __wsum l3sum = pcn_csum_diff(0, 0, (__be32 *)ip, sizeof(*ip), 0);
  pcn_l3_csum_replace(ctx, IP_CSUM_OFFSET, 0, l3sum, 0);

  return RX_OK;
}