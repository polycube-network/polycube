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

/* ===========================================
   LABEL the packet with the connection status
   =========================================== */

/* INCLUDE */

#include <uapi/linux/ip.h>

/* DEFINE */

#define AF_INET 2 /* Internet IP Protocol 	*/

#define SESSION_DIM _SESSION_DIM

/* MACRO */

#define BIT_SET(y, mask) (y |= (mask))
#define BIT_CLEAR(y, mask) (y &= ~(mask))

#define CHECK_MASK_IS_SET(y, mask) (y & mask)

/* ENUM */

// bit used in session->setMask
enum { BIT_CONNTRACK, BIT_DNAT_FWD, BIT_DNAT_REV, BIT_SNAT_FWD, BIT_SNAT_REV };

// bit used in commit->mask
enum {
  BIT_CT_SET_MASK,
  BIT_CT_CLEAR_MASK,
  BIT_CT_SET_TTL,
  BIT_CT_SET_STATE,
  BIT_CT_SET_SEQUENCE
};

/* STRUCT */

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

struct session_v {
  uint8_t setMask;     // bitmask for set fields
  uint8_t actionMask;  // bitmask for actions to be applied or not

  uint64_t ttl;
  uint8_t state;
  uint32_t sequence;

  uint32_t dnatFwdToIp;
  uint16_t dnatFwdToPort;

  uint32_t snatFwdToIp;
  uint16_t snatFwdToPort;

  uint32_t dnatRevToIp;
  uint16_t dnatRevToPort;

  uint32_t snatRevToIp;
  uint16_t snatRevToPort;

} __attribute__((packed));

/* BPF MAPS DECLARATION */

BPF_TABLE("extern", int, struct packetHeaders, packet, 1);
BPF_TABLE("extern", uint32_t, struct session_v, session, SESSION_DIM);

#if _INGRESS_LOGIC
BPF_TABLE("extern", int, uint64_t, timestamp, 1);
BPF_DEVMAP(tx_port, 128);
#endif

#if _EGRESS_LOGIC
BPF_TABLE("extern", int, uint64_t, timestamp, 1);
#endif

/* INLINE FUNCTIONS */

/* from include/net/ip.h */
static __always_inline int ip_decrease_ttl(struct iphdr *iph) {
  u32 check = (__force u32)iph->check;

  check += (__force u32)htons(0x0100);
  iph->check = (__force __sum16)(check + (check >= 0xFFFF));
  return --iph->ttl;
}

static __always_inline uint64_t *time_get_ns() {
  int key = 0;
  return timestamp.lookup(&key);
}

/* BPF PROGRAM */

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
#if _CONNTRACK_MODE == 0
  return RX_OK;
#else
  pcn_log(ctx, LOG_DEBUG, "[_HOOK] [ConntrackTableUpdate] receiving packet M ");

  struct packetHeaders *pkt;
  int k = 0;
  pkt = packet.lookup(&k);

  if (pkt == NULL) {
    // Not possible
    return RX_DROP;
  }

  pcn_log(ctx, LOG_DEBUG,
          "[_HOOK] [ConntrackTableUpdate] commit lookup succeded! ");
  pcn_log(ctx, LOG_DEBUG, "[_HOOK] [ConntrackTableUpdate] pkt->mask=0x%x ",
          pkt->mask);

  if (pkt->mask != 0) {
    struct packetHeaders *pkt;
    k = 0;
    pkt = packet.lookup(&k);

    if (pkt == NULL) {
      return RX_DROP;
    }

    struct session_v *session_value_p = 0;
    uint32_t sessionId = pkt->sessionId;

    session_value_p = session.lookup(&sessionId);
    if (session_value_p == NULL) {
      pcn_log(ctx, LOG_DEBUG,
              "[_HOOK] [ConntrackTableUpdate] lookup sessionId: %d FAILED ",
              sessionId);
      return RX_DROP;
    }

    if (CHECK_MASK_IS_SET(pkt->mask, BIT(BIT_CT_SET_STATE))) {
      session_value_p->state = pkt->state;
      pcn_log(ctx, LOG_DEBUG,
              "[_HOOK] [ConntrackTableUpdate] Committing pkt->state = %d ",
              pkt->state);
    } else {
      pcn_log(ctx, LOG_DEBUG,
              "[_HOOK] [ConntrackTableUpdate] NOT Committing pkt->state");
    }
    if (CHECK_MASK_IS_SET(pkt->mask, BIT(BIT_CT_SET_TTL))) {
      session_value_p->ttl = pkt->ttl;
      pcn_log(ctx, LOG_DEBUG,
              "[_HOOK] [ConntrackTableUpdate] Committing pkt->ttl = %d ",
              pkt->ttl);
    } else {
      pcn_log(ctx, LOG_DEBUG,
              "[_HOOK] [ConntrackTableUpdate] NOT Committing pkt->ttl");
    }
    if (CHECK_MASK_IS_SET(pkt->mask, BIT(BIT_CT_SET_SEQUENCE))) {
      session_value_p->sequence = pkt->sequence;
      pcn_log(
          ctx, LOG_DEBUG,
          "[_HOOK] [ConntrackTableUpdate] Committing pkt->sequence = %d ",
          pkt->sequence);
    } else {
      pcn_log(
          ctx, LOG_DEBUG,
          "[_HOOK] [ConntrackTableUpdate] NOT Committing pkt->sequence ");
    }
    if (CHECK_MASK_IS_SET(pkt->mask, BIT(BIT_CT_SET_MASK))) {
      BIT_SET(session_value_p->setMask, pkt->setMask);
      pcn_log(ctx, LOG_DEBUG,
              "[_HOOK] [ConntrackTableUpdate] Committing pkt->setMask = %x "
              "new setMask = %x ",
              pkt->setMask, session_value_p->setMask);
    } else {
      pcn_log(ctx, LOG_DEBUG,
              "[_HOOK] [ConntrackTableUpdate] NOT Committing pkt->setMask ");
    }
    // BIT_CT_CLEAR_MASK not currently used

    goto forward_action;
  } else {
    pcn_log(ctx, LOG_DEBUG,
            "[_HOOK] [ConntrackTableUpdate] pkt->mask == 0 -> nothing to "
            "commit ");
    goto forward_action;
  }

forward_action:;

// FIB_LOOKUP_ENABLED is replaced a runtime by control plane
// true: if kernel >= 4.19.0 && XDP mode
#if _FIB_LOOKUP_ENABLED

#if _INGRESS_LOGIC

  // use fib_lookup helper to redirect pkt to egress interface directly (if
  // possible)

  // re-parse pkt is more convinient than parse it at pipeline beginning.
  // furthermore it is needed, since we have to change the packet

  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;
  struct bpf_fib_lookup fib_params;
  struct ethhdr *eth = data;
  struct ipv6hdr *ip6h;
  struct iphdr *iph;
  u16 h_proto;
  u64 nh_off;
  int rc;

  nh_off = sizeof(*eth);
  if (data + nh_off > data_end)
    return XDP_DROP;

  __builtin_memset(&fib_params, 0, sizeof(fib_params));

  h_proto = eth->h_proto;
  if (h_proto == htons(ETH_P_IP)) {
    iph = data + nh_off;

    if (iph + 1 > data_end)
      return XDP_DROP;

    if (iph->ttl <= 1)
      return XDP_PASS;

    fib_params.family = AF_INET;
    fib_params.tos = iph->tos;
    fib_params.l4_protocol = iph->protocol;
    fib_params.sport = 0;
    fib_params.dport = 0;
    fib_params.tot_len = ntohs(iph->tot_len);
    fib_params.ipv4_src = iph->saddr;
    fib_params.ipv4_dst = iph->daddr;
  } else {
    return RX_OK;
  }

  fib_params.ifindex = ctx->ingress_ifindex;

  rc = bpf_fib_lookup(ctx, &fib_params, sizeof(fib_params), 0);

  /* verify egress index has xdp support
   * TO-DO bpf_map_lookup_elem(&tx_port, &key) fails with
   *       cannot pass map_type 14 into func bpf_map_lookup_elem#1:
   * NOTE: without verification that egress index supports XDP
   *       forwarding packets are dropped.
   */
  if (rc == 0) {
    if (h_proto == htons(ETH_P_IP))
      ip_decrease_ttl(iph);

    memcpy(eth->h_dest, fib_params.dmac, ETH_ALEN);
    memcpy(eth->h_source, fib_params.smac, ETH_ALEN);

    pcn_log(ctx, LOG_TRACE,
            "[_HOOK] [ConntrackTableUpdate] +bpf_fib_lookup+ redirect pkt to "
            "ifindex %d ",
            fib_params.ifindex);
    return tx_port.redirect_map(fib_params.ifindex, 0);

  } else {
    pcn_log(ctx, LOG_TRACE,
            "[_HOOK] [ConntrackTableUpdate] ACCEPT packet, after save in "
            "session table");
    return RX_OK;
  }
#endif

#if _EGRESS_LOGIC
  pcn_log(ctx, LOG_TRACE,
          "[_HOOK] [ConntrackTableUpdate] ACCEPT packet, after save in session "
          "table");
  return RX_OK;
#endif

#else
  pcn_log(ctx, LOG_TRACE,
          "[_HOOK] [ConntrackTableUpdate] ACCEPT packet, after save in session "
          "table");
  return RX_OK;
#endif

#endif
}
