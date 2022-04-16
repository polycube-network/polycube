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
#include <bcc/proto.h>
#include <uapi/linux/bpf.h>
#include <uapi/linux/filter.h>
#include <uapi/linux/icmp.h>
#include <uapi/linux/if_arp.h>
#include <uapi/linux/if_ether.h>
#include <uapi/linux/if_packet.h>
#include <uapi/linux/in.h>
#include <uapi/linux/ip.h>
#include <uapi/linux/pkt_cls.h>
#include <uapi/linux/udp.h>

#define CHECK_MAC_DST
#define ROUTING_TABLE_DIM 100000
#define ROUTER_PORT_N 32
#define ARP_TABLE_DIM 32
#define MAX_SECONDARY_ADDRESSES 5 // also defined in Ports.h
#define TYPE_NOLOCALINTERFACE 0  // used to compare the 'type' field in the rt_v
#define TYPE_LOCALINTERFACE 1
#define IP_CSUM_OFFSET (sizeof(struct eth_hdr) + offsetof(struct iphdr, check))
#define ICMP_CSUM_OFFSET                           \
  (sizeof(struct eth_hdr) + sizeof(struct iphdr) + \
   offsetof(struct icmphdr, checksum))
#define MAC_MULTICAST_MASK 0x1ULL  // network byte order
enum {
  SLOWPATH_ARP_REPLY = 1,
  SLOWPATH_ARP_LOOKUP_MISS,
  SLOWPATH_TTL_EXCEEDED,
  SLOWPATH_PKT_FOR_ROUTER
};
/* Routing Table Key */
struct rt_k {
  u32 netmask_len;
  __be32 network;
};
/* Routing Table Value
the type field is used to know if the destination is one interface of the router
*/
struct rt_v {
  u32 port;
  __be32 nexthop;
  u8 type;
};
/* Router Port, also defined in Ports.h */
struct r_port {
  __be32 ip;
  __be32 netmask;
  __be32 secondary_ip[MAX_SECONDARY_ADDRESSES];
  __be32 secondary_netmask[MAX_SECONDARY_ADDRESSES];
  __be64 mac : 48;
};
BPF_F_TABLE("lpm_trie", struct rt_k, struct rt_v, routing_table,
            ROUTING_TABLE_DIM, BPF_F_NO_PREALLOC);
/*
Router Port table provides a way to simulate the physical interface of the
router
The ip address is used to answer to the arp request (TO IMPLEMENT)
The mac address is used as mac_src for the outcoming packet on that interface,
and as mac address contained in the arp reply
*/
BPF_TABLE("hash", u16, struct r_port, router_port, ROUTER_PORT_N);
/*
Arp Table implements a mapping between IP and MAC addresses.
*/
struct arp_entry {
  __be64 mac;
  u32 port;
} __attribute__((packed));
BPF_TABLE("hash", u32, struct arp_entry, arp_table, ARP_TABLE_DIM);

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
/*the function checks if the packet is an ICMP ECHO REQUEST and source mac is
* not equal to in_port mac, if it is true sends the
* packet to the slowpath. The slowpath searchs if the destination ip is one of
* ip in the router and generates an echo reply
*/
static inline int send_packet_for_router_to_slowpath(struct CTXTYPE *ctx,
                                                     struct pkt_metadata *md,
                                                     struct eth_hdr *eth,
                                                     struct iphdr *ip) {
  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;
  struct icmphdr *icmp = data + sizeof(*eth) + sizeof(*ip);
  if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*icmp) > data_end)
    return RX_DROP;
  u32 mdata[3];
  mdata[0] = ip->saddr;
  mdata[1] = ip->daddr;
  mdata[2] = ip->protocol;
  pcn_pkt_controller_with_metadata(ctx, md, SLOWPATH_PKT_FOR_ROUTER, mdata);
  return RX_DROP;
}
static inline int send_icmp_ttl_time_exceeded(struct CTXTYPE *ctx,
                                              struct pkt_metadata *md,
                                              __be32 ip_port) {
  pcn_log(ctx, LOG_DEBUG, "packet DROP (ttl = 0)");
  // Set router port ip address as metadata[0]
  u32 mdata[3];
  // using the principal ip as sender address
  mdata[0] = ip_port;
  // Send packet to slowpath
  pcn_pkt_controller_with_metadata(ctx, md, SLOWPATH_TTL_EXCEEDED, mdata);
  return RX_DROP;
}
static inline int arp_lookup_miss(struct CTXTYPE *ctx, struct pkt_metadata *md,
                                  __be32 dst_ip, u16 out_port, __be32 ip_port) {
  pcn_log(ctx, LOG_DEBUG, "arp lookup failed. Send to controller");

  // Set metadata and send packet to slowpath
  u32 mdata[3];
  mdata[0] = dst_ip;
  mdata[1] = out_port;
  mdata[2] = ip_port;
  pcn_pkt_controller_with_metadata(ctx, md, SLOWPATH_ARP_LOOKUP_MISS, mdata);
  return RX_DROP;
}
static inline __u16 checksum(unsigned short *buf, int bufsz) {
  unsigned long sum = 0;

  while (bufsz > 1) {
    sum += *buf;
    buf++;
    bufsz -= 2;
  }
  if (bufsz == 1) {
    sum += *(unsigned char *)buf;
  }
  sum = (sum & 0xffff) + (sum >> 16);
  // sum = (sum & 0xffff) + (sum >> 16);
  return ~sum;
}

static inline int send_packet_to_output_interface(
    struct CTXTYPE *ctx, struct pkt_metadata *md, struct eth_hdr *eth,
    struct iphdr *ip, __be32 nexthop, u16 out_port, __be32 ip_port,
    __be64 mac_port) {
  __be32 dst_ip = 0;
  if (nexthop == 0)
    // Next Hop is local, directly lookup in arp table for the destination ip.
    dst_ip = ip->daddr;
  else
    // Next Hop not local, lookup in arp table for the next hop ip address.
    dst_ip = nexthop;
  struct arp_entry *entry = arp_table.lookup(&dst_ip);
  if (!entry)
    return arp_lookup_miss(ctx, md, dst_ip, out_port, ip_port);

  pcn_log(ctx, LOG_TRACE, "in: %d out: %d REDIRECT", md->in_port, out_port);

  __be32 l3sum = 0;
  // eth->dst = *mac_entry;
  eth->dst = entry->mac;
  eth->src = mac_port;
  /* Decrement TTL and update checksum */
  __u32 new_ttl = ip->ttl - 1;

#ifdef POLYCUBE_XDP
  ip->check = 0;
  ip->ttl = (__u8)new_ttl;
  ip->check = checksum((unsigned short *)ip, sizeof(struct iphdr));
#else
  __u32 old_ttl = ip->ttl;
  l3sum = bpf_csum_diff(&old_ttl, 4, &new_ttl, 4, l3sum);
  ip->ttl = (__u8)new_ttl;
  bpf_l3_csum_replace(ctx, IP_CSUM_OFFSET, 0, l3sum, 0);
#endif

  return pcn_pkt_redirect(ctx, md, out_port);
}
static inline int search_secondary_address(__be32 *arr, __be32 ip) {
  int i, size = MAX_SECONDARY_ADDRESSES;
  for (i = 0; i < size; i++) {
    if (arr[i] == ip)
      return i; /* found */
  }
  return (-1); /* if it was not found */
}
/*when an arp request is received, the router controls if the target if is one
* of its interfaces,
* if it is true, it sends an arp reply to the ingress port
*/
static inline int send_arp_reply(struct CTXTYPE *ctx, struct pkt_metadata *md,
                                 struct eth_hdr *eth, struct arp_hdr *arp,
                                 struct r_port *in_port) {
  __be32 target_ip = arp->ar_tip;
  __be32 sender = 0;
  if (target_ip == in_port->ip)
    sender = in_port->ip;
  else {
    __be32 *arr = in_port->secondary_ip;
    int pos = search_secondary_address(arr, target_ip);
    if (pos > -1)
      sender = arr[pos];
    else
      return RX_DROP;
  }

  pcn_log(ctx, LOG_DEBUG, "somebody is asking for my address");

  __be64 remotemac = arp->ar_sha;
  __be32 remoteip = arp->ar_sip;
  arp->ar_op = bpf_htons(ARPOP_REPLY);
  arp->ar_tha = remotemac;
  arp->ar_sha = in_port->mac;
  arp->ar_sip = sender;
  arp->ar_tip = remoteip;
  eth->dst = remotemac;
  eth->src = in_port->mac;
  /* register the requesting mac and ip */
  struct arp_entry entry;
  entry.mac = remotemac;
  entry.port = md->in_port;
  arp_table.update(&remoteip, &entry);
  return pcn_pkt_redirect(ctx, md, md->in_port);
}
static inline int notify_arp_reply_to_slowpath(struct CTXTYPE *ctx,
                                               struct pkt_metadata *md,
                                               struct arp_hdr *arp) {
  pcn_log(ctx, LOG_DEBUG, "packet is arp reply");

  __be64 mac_ = arp->ar_sha;
  __be32 ip_ = arp->ar_sip;
  struct arp_entry entry;
  entry.mac = mac_;
  entry.port = md->in_port;
  arp_table.update(&ip_, &entry);
  // notify the slowpath. New arp reply received.
  u32 mdata[3];
  mdata[0] = ip_;
  pcn_pkt_controller_with_metadata(ctx, md, SLOWPATH_ARP_REPLY, mdata);
  return RX_DROP;
}
static inline int is_ether_mcast(__be64 mac_address) {
  return (mac_address & (__be64)MAC_MULTICAST_MASK);
}
static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;
  struct eth_hdr *eth = data;
  if (data + sizeof(*eth) > data_end)
    goto DROP;

  pcn_log(ctx, LOG_TRACE, "in_port: %d, proto: 0x%x, mac_src: %M mac_dst: %M",
          md->in_port, bpf_htons(eth->proto), eth->src, eth->dst);

  u16 in_port_index = md->in_port;
  struct r_port *in_port = router_port.lookup(&in_port_index);
  if (!in_port) {
    pcn_log(ctx, LOG_ERR, "received packet from non valid port: %d",
            md->in_port);
    goto DROP;
  }
/*
Check if the mac destination of the packet is multicast, broadcast, or the
unicast address of the router port.  If not, drop the packet.
*/
#ifdef CHECK_MAC_DST
  if (eth->dst != in_port->mac && !is_ether_mcast(eth->dst)) {
    pcn_log(ctx, LOG_DEBUG, "mac destination %M MISMATCH %M", eth->dst,
            in_port->mac);
    goto DROP;
  }
#endif
  switch (eth->proto) {
  case htons(ETH_P_IP):
    goto IP;  // ipv4 packet
  case htons(ETH_P_ARP):
    goto ARP;  // arp packet
  default:
    goto DROP;
  }
IP:;  // ipv4 packet
  struct iphdr *ip = data + sizeof(*eth);
  if (data + sizeof(*eth) + sizeof(*ip) > data_end)
    goto DROP;

  pcn_log(ctx, LOG_TRACE, "ttl: %u", ip->ttl);

  // find entry in routing table
  struct rt_k k = {32, ip->daddr};
  struct rt_v *rt_entry_p = routing_table.lookup(&k);
  if (!rt_entry_p) {
    pcn_log(ctx, LOG_TRACE, "no routing table match for %I", ip->daddr);
    goto DROP;
  }
  /* Check if the pkt destination is one local interface of the router */
  if (rt_entry_p->type == TYPE_LOCALINTERFACE) {
#ifdef SHADOW
    return pcn_pkt_redirect_ns(ctx, md, md->in_port);
#endif
    return send_packet_for_router_to_slowpath(ctx, md, eth, ip);
  }
  if (ip->ttl == 1) {
    return send_icmp_ttl_time_exceeded(ctx, md, in_port->ip);
  }
  // Select out interface
  u16 out_port = rt_entry_p->port;
  struct r_port *r_port_p = router_port.lookup(&out_port);
  if (!r_port_p) {
    pcn_log(ctx, LOG_ERR, "out port '%d' not found", out_port);
    goto DROP;
  }
  // redirect packet to out interface
  return send_packet_to_output_interface(ctx, md, eth, ip, rt_entry_p->nexthop,
                                         out_port, r_port_p->ip, r_port_p->mac);
ARP:;  // arp packet
  struct arp_hdr *arp = data + sizeof(*eth);
  if (data + sizeof(*eth) + sizeof(*arp) > data_end)
    goto DROP;
  if (arp->ar_op == bpf_htons(ARPOP_REQUEST)) { // arp request?
#ifdef SHADOW
    return pcn_pkt_redirect_ns(ctx, md, md->in_port);
#endif
    return send_arp_reply(ctx, md, eth, arp, in_port);
  } else if (arp->ar_op == bpf_htons(ARPOP_REPLY))  // arp reply
    return notify_arp_reply_to_slowpath(ctx, md, arp);
  return RX_DROP;
DROP:
  pcn_log(ctx, LOG_TRACE, "in: %d out: -- DROP", md->in_port);
  return RX_DROP;
}
