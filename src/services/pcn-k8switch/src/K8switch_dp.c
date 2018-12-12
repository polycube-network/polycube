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


#pragma once

#include <string>

#include <uapi/linux/if_arp.h>
#include <uapi/linux/ip.h>
#include <uapi/linux/tcp.h>
#include <uapi/linux/udp.h>
#include <linux/jhash.h>

#define SERVICES_MAP_DIM 65536
#define NODEPORT_SESSION_DIM 65536

#define IP_CSUM_OFFSET (sizeof(struct eth_hdr) + offsetof(struct iphdr, check))
#define TCP_CSUM_OFFSET (sizeof(struct eth_hdr) + sizeof(struct iphdr) + offsetof(struct tcphdr, check))
#define UDP_CSUM_OFFSET (sizeof(struct eth_hdr) + sizeof(struct iphdr) + offsetof(struct udphdr, check))
#define IS_PSEUDO 0x10


#ifndef NODEPORT_PORT
#define NODEPORT_PORT 0xFFFF
#endif

#ifndef CLIENT_SUBNET_MASK
#define CLIENT_SUBNET_MASK 0UL
#endif

#ifndef CLIENT_SUBNET
#define CLIENT_SUBNET 0UL
#endif

#ifndef VIRTUAL_CLIENT_SUBNET
#define VIRTUAL_CLIENT_SUBNET 0UL
#endif

#ifndef CLUSTER_IP_SUBNET
#define CLUSTER_IP_SUBNET bpf_htonl(0x0a600000UL)    // 10.96.0.0/12
#define CLUSTER_IP_MASK bpf_htonl(0xfff00000UL)   // /12 -> 255.240.0.0
#endif

#define KUBE_PROXY_FLAG (65535U)
#define KUBE_PROXY_SERVICE (0x1)

enum {
  CLUSTER_IP_IN = 1,
  NODE_PORT_IN = 2,
  FROM_CLUSTER_IP_SERVICE = 3,
  FROM_NODE_PORT_SERVICE = 4,
};

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

// This struct keeps the session ID of the traffic that has currently been
// handled by the load balancer.
struct session {
  __be32 ip_src;
  __be32 ip_dst;
  __be16 port_src;
  __be16 port_dst;
  __be16 proto;
} __attribute__((packed));

struct vip {
  __be32 ip;
  __be16 port;
  __be16 proto;

  __be16 index;  //refers to backend's index for that Service(vip)
  u16 pad0;
  u16 pad1;
  u16 pad2;
} __attribute__((packed));

struct backend {
  __be32 ip;
  __be16 port;
  __be16 proto;
} __attribute__((packed));

/*
 * This table contains multiple entries for each virtual service (i.e., virtual IP / protocol / port).
 * When 'index' = 0, the value in the hash map is actually not a couple
 *   IP address / port but the number of backends (also called 'pool size')
 *   associated to that virtual service.
 * Then, for following entries (i.e., when 'index' > 0), the
 *   'struct backend' keeps the actual data (IP/port) for that backend.
 *
 * Example:
 *   VIP 1, index 0 -> Number of backends: 4
 *   VIP 1, index 1 -> Backend 2
 *   VIP 1, index 2 -> Backend 1
 *   VIP 1, index 3 -> Backend 2
 *   VIP 1, index 4 -> Backend 1
 *
 *   VIP 2, index 0 -> Number of backends: 2
 *   VIP 2, index 1 -> Backend 1
 *   VIP 2, index 2 -> Backend 1
 *
 *   where VIP 1 has 4 backends and VIP2 has 2 backend.
 *
 * The number of backend in this table may be different from the number of
 * actual servers in order to keep session affinity when a server is
 * added/removed, without having to move the traffic from a backend#1
 * to a backend #2 (which will break the TCP session)
 *
 * The load balancing algorith will select the proper backend based on an hash
 * on the most significant fields (i.e., session id) of the packet:
 *
 *   __u32 backend_id = session_hash % pool_size + 1;
 *
 * where 'pool_size' is 4 for VIP 1.
 */

// maps for ClusterIP services
BPF_TABLE("hash", struct vip, struct backend, services, SERVICES_MAP_DIM);

// reverse map to get virtual IP and Vport from real backend
BPF_TABLE("hash", struct backend, struct vip, cluster_ip_reverse, SERVICES_MAP_DIM);

struct nodeport_session_in {
  __be32 ip_src;
  __be32 ip_dst;
  __be16 port_src;
  __be16 port_dst;
  __be16 proto;
  u64 timestamp;
} __attribute__((packed));

struct nodeport_session_out {
  __be64 mac_dst; // : 48;
  __be64 mac_src; // : 48;
  __be32 ip_src;
  __be32 ip_dst;
  __be16 port_src;
  __be16 port_dst;
  __be16 proto;
  u64 timestamp;
} __attribute__((packed));
BPF_TABLE("hash", struct nodeport_session_in, struct nodeport_session_out, nodeport_session_out, NODEPORT_SESSION_DIM);
BPF_TABLE("hash", struct nodeport_session_out, struct nodeport_session_in, nodeport_session_in, NODEPORT_SESSION_DIM);

// Saves IP to MAC assosiation for pods running in the local node
struct pod {
  __be64 mac;
  u16 port;
};

BPF_TABLE("array", u32, struct pod, fwd_table, 256);

// buffer for circular array holding the free ports
BPF_TABLE("percpu_array", u32, u64, free_ports_buffer, 65536);

struct cb_control {
  u16 reader;
  u16 writer;
  u32 size;
} __attribute__((packed));
BPF_TABLE("array", u32, struct cb_control, free_ports_cb, 1024);

// allocates and returns a free port to create a new session
static __always_inline
__be16 get_free_port(struct CTXTYPE *ctx) {
  u32 cpu_index = bpf_get_smp_processor_id();
  //pcn_log(ctx, LOG_TRACE, "get port on core %u", cpu_index);
  struct cb_control *ctrl = free_ports_cb.lookup(&cpu_index);
  if (!ctrl) {
    pcn_log(ctx, LOG_ERR, "Ctrl structure not found");
    return 0;
  }

  //pcn_log(ctx, LOG_TRACE, "reader: %u, writer: %u, size: %u", ctrl->reader, ctrl->writer, ctrl->size);

  if (ctrl->reader == ctrl->writer) {
    pcn_log(ctx, LOG_ERR, "There are not free ports for this core");
    return 0;
  }

  // at this point we are sure the buffer is not empty, let's take an element
  u32 reader = ctrl->reader;
  ctrl->reader++;
  ctrl->reader %= ctrl->size;

  u64 *port = free_ports_buffer.lookup(&reader);
  if (!port) {
    pcn_log(ctx, LOG_ERR, "BUG: port not found at position %u", reader);
    return 0;
  }

  return bpf_htons(*port);
}

// get real IP and port for a clusterIP service
static __always_inline
struct backend *get_cluster_ip_backend(struct CTXTYPE *ctx,
                              __be32 ip_src,
                              __be32 ip_dst,
                              __be16 port_src,
                              __be16 port_dst,
                              __be16 proto,
                              u16 *action) {

  // check if there is a service for this
  struct vip v_key = {
    .ip = ip_dst,
    .port = port_dst,
    .proto = proto,
    .index = 0,
  };

  struct backend *backend_value = services.lookup(&v_key);
  if (!backend_value) {
    pcn_log(ctx, LOG_TRACE, "Service lookup failed.");
    return NULL;
  }

  // there are backends for this service?
  u32 pool_size = backend_value->port;
  if (!pool_size) {
    return NULL;
  }

  // is the service managed by kube-proxy?
  if (pool_size == KUBE_PROXY_FLAG) {
    *action = KUBE_PROXY_SERVICE;
    return NULL;
  }

  struct session sessions_key = {
    .ip_src = ip_src,
    .ip_dst = ip_dst,
    .port_src = port_src,
    .port_dst = port_dst,
    .proto = proto,
  };

  u32 check = jhash((const void *)&sessions_key, sizeof(struct session),
                    JHASH_INITVAL);
  u32 id = check % pool_size + 1;
  pcn_log(ctx, LOG_TRACE, "Backend lookup ip: %I, port: %P, proto: 0x%04x, id: %d", ip_dst, port_dst, proto, id);

  v_key.index = id;

  // get the backend using the index
  return services.lookup(&v_key);
}

static __always_inline
int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  u8 type = 0;
  u8 outport;
  u16 action = 0;

  __u16 ip_proto = 0;
  __be32 l4csum_offset = 0;
  __be16 *dport;
  __be16 *sport;
  struct backend* bck_value;

  __be32 l3sum = 0;
  __be32 l4sum = 0;

  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;

  struct eth_hdr *eth = data;
  if (data + sizeof(*eth) > data_end)
    goto DROP;

  switch (eth->proto) {
  case htons(ETH_P_IP):
    goto IP;
  case htons(ETH_P_ARP):
    pcn_log(ctx, LOG_DEBUG, "Arp packet received");
  default:
    goto DROP;
  }

IP:;
  struct iphdr *ip = data + sizeof(*eth);
  if (data + sizeof(*eth) + sizeof(*ip) > data_end)
    goto DROP;

  // Where is the packet going to?
  if ((ip->daddr & CLUSTER_IP_MASK) == CLUSTER_IP_SUBNET) {  // a clusterIP service?
    pcn_log(ctx, LOG_TRACE, "Packet going to a ClusterIP service");
    type = CLUSTER_IP_IN;
  } else if ((ip->daddr & CLIENT_SUBNET_MASK) == VIRTUAL_CLIENT_SUBNET) { // a local pod's rewritten IP?
    if ((ip->daddr & ~CLIENT_SUBNET_MASK) == bpf_htonl(0x2UL)) { // is this a response for a NodePort?
      pcn_log(ctx, LOG_TRACE, "Packet comming from a NodePort service");
      type = FROM_NODE_PORT_SERVICE;
    } else {
      pcn_log(ctx, LOG_TRACE, "Packet comming from a ClusterIP service");
      type = FROM_CLUSTER_IP_SERVICE;
    }
  } else if (md->in_port == NODEPORT_PORT) { // packet going to a node port service
    pcn_log(ctx, LOG_TRACE, "Packet going to a NodePort service");
    type = NODE_PORT_IN;
  } else {  // anywhere else?
    goto FORWARD;
  }

  switch (ip->protocol) {
  case IPPROTO_UDP: ;
    struct udphdr *udp = data + sizeof(struct eth_hdr) + sizeof(struct iphdr);
    if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*udp) > data_end)
      goto DROP;

    pcn_log(ctx, LOG_TRACE, "received UDP: %I:%P --> %I:%P", ip->saddr, udp->source, ip->daddr, udp->dest);

    dport = &udp->dest;
    sport = &udp->source;
    ip_proto = htons(IPPROTO_UDP);

    l4csum_offset = UDP_CSUM_OFFSET;
    break;

  case IPPROTO_TCP: ;
    struct tcphdr *tcp = data + sizeof(struct eth_hdr) + sizeof(struct iphdr);
    if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*tcp) > data_end)
      goto DROP;

    pcn_log(ctx, LOG_TRACE, "received TCP: %I:%P --> %I:%P", ip->saddr, tcp->source, ip->daddr, tcp->dest);

    dport = &tcp->dest;
    sport = &tcp->source;
    ip_proto = htons(IPPROTO_TCP);

    l4csum_offset = TCP_CSUM_OFFSET;
    break;

  default:
    goto FORWARD;
  }

  switch (type) {
  case CLUSTER_IP_IN: {
    bck_value = get_cluster_ip_backend(ctx,
      ip->saddr, ip->daddr, *sport, *dport, ip_proto, &action);

    if (!bck_value) {
      if (action == KUBE_PROXY_SERVICE) {
        pcn_log(ctx, LOG_TRACE, "Service managed by kube-proxy, forwarding packet");
        goto FORWARD;
      }
      goto DROP;
    }

    pcn_log(ctx, LOG_TRACE, "Found backend with ip: %I and port: %P", bck_value->ip, bck_value->port);

    // change destination IP for backend
    l3sum = pcn_csum_diff(&ip->daddr, 4, &bck_value->ip, 4, l3sum);
    ip->daddr = bck_value->ip;

    // rewrite source IP
    __be32 old = ip->saddr;
    ip->saddr &= ~CLIENT_SUBNET_MASK;
    ip->saddr |= VIRTUAL_CLIENT_SUBNET;
    l3sum = pcn_csum_diff(&old, 4, &ip->saddr, 4, l3sum);

    // change destination port to backend one
    __be32 old_port = *dport;
    __be32 new_port = bck_value->port;
    *dport = bck_value->port;
    l4sum = pcn_csum_diff(&old_port, 4, &new_port, 4, l4sum);
    pcn_log(ctx, LOG_TRACE, "redirected to %I:%P --> %I:%P", ip->saddr, *sport, ip->daddr, *dport);
    goto FORWARD_CSUM;
  }

  case FROM_CLUSTER_IP_SERVICE: {
    // The packet is comming from a service
    // -> change the destination IP to the client one
    // -> change the source IP and port to the VIP
    struct backend key = {.ip = ip->saddr,
                          .port = *sport,
                          .proto = ip_proto};

    struct vip* vip_v = cluster_ip_reverse.lookup(&key);
    if (!vip_v) {
      pcn_log(ctx, LOG_ERR, "Reverse backend lookup failed");
      goto DROP;
    }

    __be32 old;

    // rewrite destination IP to real IP of the client
    old = ip->daddr;
    ip->daddr &= ~CLIENT_SUBNET_MASK;
    ip->daddr |= CLIENT_SUBNET;
    l3sum = pcn_csum_diff(&old, 4, &ip->daddr, 4, l3sum);

    // change source IP to VIP
    l3sum = pcn_csum_diff(&ip->saddr, 4, &vip_v->ip, 4, l3sum);
    ip->saddr = vip_v->ip;

    // change source port to Vport
    __be32 old_port = *sport;
    __be32 new_port = vip_v->port;
    *sport = vip_v->port;
    l4sum = pcn_csum_diff(&old_port, 4, &new_port, 4, l4sum);
    pcn_log(ctx, LOG_TRACE, "redirected to %I:%P --> %I:%P", ip->saddr, *sport, ip->daddr, *dport);
    goto FORWARD_CSUM;
  }

  case NODE_PORT_IN: {
    // original values of the packet
    struct nodeport_session_out value = {
      .mac_dst = eth->dst,
      .mac_src = eth->src,
      .ip_src = ip->saddr,
      .ip_dst = ip->daddr,
      .port_src = *sport,
      .port_dst = *dport,
      .proto = ip_proto,
      .timestamp = 0LL,
    };

    // is there a session entry for this packet?
    struct nodeport_session_in *session = nodeport_session_in.lookup(&value);
    if (!session) {
      // no, try to choose a backend
      bck_value = get_cluster_ip_backend(ctx,
        ip->saddr, ip->daddr, *sport, *dport, ip_proto, &action);

      if (!bck_value) {
        if (action == KUBE_PROXY_SERVICE) {
          pcn_log(ctx, LOG_TRACE, "Service managed by kube-proxy, forwarding packet");
          return RX_OK;
        }

        return RX_OK;
      }

      pcn_log(ctx, LOG_TRACE, "Found backend with ip: %I and port: %P", bck_value->ip, bck_value->port);

      // change destination IP for backend
      l3sum = pcn_csum_diff(&ip->daddr, 4, &bck_value->ip, 4, l3sum);
      ip->daddr = bck_value->ip;

      // change destination port to backend one
      __be32 old_port = *dport;
      __be32 new_port = bck_value->port;
      *dport = bck_value->port;
      l4sum = pcn_csum_diff(&old_port, 4, &new_port, 4, l4sum);

      // select new source port
      old_port = *sport;
      new_port = get_free_port(ctx);
      if (new_port == 0) {
        goto DROP;
      }
      *sport = new_port;
      l4sum = pcn_csum_diff(&old_port, 4, &new_port, 4, l4sum);

      // rewrite source IP
      __be32 old = ip->saddr;
      ip->saddr = VIRTUAL_CLIENT_SUBNET + bpf_htonl(2UL);
      l3sum = pcn_csum_diff(&old, 4, &ip->saddr, 4, l3sum);

      u64 timestamp = bpf_ktime_get_ns();

      // new values of the packet, will be used in the reverse direction
      struct nodeport_session_in key = {
        .ip_src = ip->saddr,
        .ip_dst = ip->daddr,
        .port_src = *sport,
        .port_dst = *dport,
        .proto = ip_proto,
        .timestamp = 0LL,
      };

      // create incommming entry
      key.timestamp = timestamp;
      nodeport_session_in.update(&value, &key);

      // create outgoing entry
      key.timestamp = 0LL;
      value.timestamp = timestamp;
      nodeport_session_out.update(&key, &value);
    } else {
      pcn_log(ctx, LOG_TRACE, "Session found");

      // change destination IP for backend
      l3sum = pcn_csum_diff(&ip->daddr, 4, &session->ip_dst, 4, l3sum);
      ip->daddr = session->ip_dst;

      // TODO: get address from session table?
      // rewrite source IP
      __be32 old = ip->saddr;
      ip->saddr = VIRTUAL_CLIENT_SUBNET + bpf_htonl(2UL);
      l3sum = pcn_csum_diff(&old, 4, &ip->saddr, 4, l3sum);

      // change destination port to backend one
      __be32 old_port = *dport;
      __be32 new_port = session->port_dst;
      *dport = session->port_dst;
      l4sum = pcn_csum_diff(&old_port, 4, &new_port, 4, l4sum);

      // select new source port
      old_port = *sport;
      new_port = session->port_src;
      *sport = session->port_src;
      l4sum = pcn_csum_diff(&old_port, 4, &new_port, 4, l4sum);

      // update timestamp
      session->timestamp = bpf_ktime_get_ns();
    }

    pcn_log(ctx, LOG_TRACE, "redirected to %I:%P --> %I:%P", ip->saddr, *sport, ip->daddr, *dport);
    goto FORWARD_CSUM;
  }

  break;

  case FROM_NODE_PORT_SERVICE: {
    // lookup in session table, notice that source and destination are exchanged
    struct nodeport_session_in key = {
      .ip_src = ip->daddr,
      .ip_dst = ip->saddr,
      .port_src = *dport,
      .port_dst = *sport,
      .proto = ip_proto,
      .timestamp = 0LL,
    };

    struct nodeport_session_out *value = nodeport_session_out.lookup(&key);
    if (!value) {
      pcn_log(ctx, LOG_ERR, "nodeport session lookup failed");
      goto DROP;
    }

    value->timestamp = bpf_ktime_get_ns();

    // rewrite destination IP to real IP of the client
    l3sum = pcn_csum_diff(&ip->daddr, 4, &value->ip_src, 4, l3sum);
    ip->daddr = value->ip_src;

    // change source IP to VIP
    l3sum = pcn_csum_diff(&ip->saddr, 4, &value->ip_dst, 4, l3sum);
    ip->saddr = value->ip_dst;

    // change source port to Vport
    __be32 old_port = *sport;
    __be32 new_port = value->port_dst;
    *sport = value->port_dst;
    l4sum = pcn_csum_diff(&old_port, 4, &new_port, 4, l4sum);

    old_port = *dport;
    new_port = value->port_src;
    *dport = value->port_src;
    l4sum = pcn_csum_diff(&old_port, 4, &new_port, 4, l4sum);

    eth->dst = value->mac_src;
    eth->src = value->mac_dst;

    pcn_log(ctx, LOG_TRACE, "redirected to %I:%P --> %I:%P", ip->saddr, *sport, ip->daddr, *dport);

    pcn_l4_csum_replace(ctx, l4csum_offset, 0, l3sum, IS_PSEUDO | 0);
    pcn_l4_csum_replace(ctx, l4csum_offset, 0, l4sum, 0);
    pcn_l3_csum_replace(ctx, IP_CSUM_OFFSET , 0, l3sum, 0);

    return pcn_pkt_redirect(ctx, md, NODEPORT_PORT);
  }
  break;

  default:
    goto DROP;
  }

FORWARD:  {
    // change destination mac in case pod is local
    __be32 dst_ip = 0;  // 0 means GW
    if ((ip->daddr & CLIENT_SUBNET_MASK) == CLIENT_SUBNET) {
      dst_ip = bpf_htonl(ip->daddr & ~CLIENT_SUBNET_MASK);
    }
    struct pod *p = fwd_table.lookup(&dst_ip);
    if (!p) {
      pcn_log(ctx, LOG_ERR, "mac not found for %u", dst_ip);
      goto DROP;
    }
    eth->dst = p->mac;
    return pcn_pkt_redirect(ctx, md, p->port);
  }

FORWARD_CSUM: {
    // change destination mac in case pod is local
    __be32 dst_ip = 0;  // 0 means GW
    if ((ip->daddr & CLIENT_SUBNET_MASK) == CLIENT_SUBNET) {
      dst_ip = bpf_htonl(ip->daddr & ~CLIENT_SUBNET_MASK);
    }
    struct pod *p = fwd_table.lookup(&dst_ip);
    if (!p) {
      pcn_log(ctx, LOG_ERR, "mac not found for %u", dst_ip);
      goto DROP;
    }
    eth->dst = p->mac;

    pcn_l4_csum_replace(ctx, l4csum_offset, 0, l3sum, IS_PSEUDO | 0);
    pcn_l4_csum_replace(ctx, l4csum_offset, 0, l4sum, 0);
    pcn_l3_csum_replace(ctx, IP_CSUM_OFFSET , 0, l3sum, 0);

    // TODO: Change source?
    return pcn_pkt_redirect(ctx, md, p->port);
  }

DROP:
  pcn_log(ctx, LOG_TRACE, "DROP packet (port = %d)", md->in_port);
  return RX_DROP;
}
