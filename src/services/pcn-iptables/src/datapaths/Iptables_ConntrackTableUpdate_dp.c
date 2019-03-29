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

#include <uapi/linux/ip.h>

#define IPPROTO_TCP 6
#define IPPROTO_UDP 17
#define IPPROTO_ICMP 1

#define ICMP_ECHOREPLY 0       /* Echo Reply			*/
#define ICMP_ECHO 8            /* Echo Request			*/
#define ICMP_TIMESTAMP 13      /* Timestamp Request		*/
#define ICMP_TIMESTAMPREPLY 14 /* Timestamp Reply		*/
#define ICMP_INFO_REQUEST 15   /* Information Request		*/
#define ICMP_INFO_REPLY 16     /* Information Reply		*/
#define ICMP_ADDRESS 17        /* Address Mask Request		*/
#define ICMP_ADDRESSREPLY 18   /* Address Mask Reply		*/

// ns
#define UDP_ESTABLISHED_TIMEOUT 180000000000
#define UDP_NEW_TIMEOUT 30000000000
#define ICMP_TIMEOUT 30000000000
#define TCP_ESTABLISHED 432000000000000
#define TCP_SYN_SENT 120000000000
#define TCP_SYN_RECV 60000000000
#define TCP_LAST_ACK 30000000000
#define TCP_FIN_WAIT 120000000000
#define TCP_TIME_WAIT 120000000000

#define TCPHDR_FIN 0x01
#define TCPHDR_SYN 0x02
#define TCPHDR_RST 0x04
#define TCPHDR_ACK 0x10

#define HEX_BE_ONE 0x1000000

#define AF_INET 2 /* Internet IP Protocol 	*/

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

enum {
  NEW,
  ESTABLISHED,
  RELATED,
  INVALID,
  SYN_SENT,
  SYN_RECV,
  FIN_WAIT_1,
  FIN_WAIT_2,
  LAST_ACK,
  TIME_WAIT
};

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
} __attribute__((packed));

struct ct_k {
  uint32_t srcIp;
  uint32_t dstIp;
  uint8_t l4proto;
  uint16_t srcPort;
  uint16_t dstPort;
} __attribute__((packed));

struct ct_v {
  uint64_t ttl;
  uint8_t state;
  uint8_t ipRev;
  uint8_t portRev;
  uint32_t sequence;
} __attribute__((packed));

#if _INGRESS_LOGIC
BPF_TABLE_SHARED("percpu_array", int, uint64_t, timestamp, 1);
BPF_DEVMAP(tx_port, 128);
#endif

#if _EGRESS_LOGIC
BPF_TABLE("extern", int, uint64_t, timestamp, 1);
#endif

BPF_TABLE("extern", struct ct_k, struct ct_v, connections, 65536);

BPF_TABLE("extern", int, struct packetHeaders, packet, 1);

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

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
// Conntrack DISABLED
#if _CONNTRACK_MODE == 0
  return RX_OK;

#else
  // Conntrack ENABLED
  struct packetHeaders *pkt;
  int k = 0;
  pkt = packet.lookup(&k);

  if (pkt == NULL) {
    // Not possible
    return RX_DROP;
  }

  pcn_log(ctx, LOG_DEBUG,
          "[ConntrackTableUpdate] received packet. SrcIP: %I, Flags: %x",
          pkt->srcIp, pkt->flags);
  pcn_log(ctx, LOG_DEBUG,
          "[ConntrackTableUpdate] received packet. SeqN: %u, AckN: %u. "
          "ConnStatus: %u",
          pkt->seqN, pkt->ackN, pkt->connStatus);

  if (pkt->connStatus == INVALID) {
    // No business here for these packets
    goto forward_action;
  }

  struct ct_k key = {0, 0, 0, 0, 0};
  uint8_t ipRev = 0;
  uint8_t portRev = 0;

  if (pkt->srcIp <= pkt->dstIp) {
    key.srcIp = pkt->srcIp;
    key.dstIp = pkt->dstIp;
    ipRev = 0;
  } else {
    key.srcIp = pkt->dstIp;
    key.dstIp = pkt->srcIp;
    ipRev = 1;
  }

  key.l4proto = pkt->l4proto;

  if (pkt->srcPort < pkt->dstPort) {
    key.srcPort = pkt->srcPort;
    key.dstPort = pkt->dstPort;
    portRev = 0;
  } else if (pkt->srcPort > pkt->dstPort) {
    key.srcPort = pkt->dstPort;
    key.dstPort = pkt->srcPort;
    portRev = 1;
  } else {
    key.srcPort = pkt->srcPort;
    key.dstPort = pkt->dstPort;
    portRev = ipRev;
  }

  struct ct_v newEntry = {0, 0, 0, 0, 0};
  struct ct_v *value;

  uint64_t *timestamp;
  timestamp = time_get_ns();

  if (timestamp == NULL) {
    pcn_log(ctx, LOG_DEBUG, "[TableUpdate] timestamp NULL. ret DROP ");
    return RX_DROP;
  }

  /* == TCP  == */
  if (pkt->l4proto == IPPROTO_TCP) {
    // If it is a RST, label it as established.
    if ((pkt->flags & TCPHDR_RST) != 0) {
      // connections.delete(&key);
      goto forward_action;
    }

    value = connections.lookup(&key);
    if (value != NULL) {
      if ((value->ipRev == ipRev) && (value->portRev == portRev)) {
        goto TCP_FORWARD;
      } else if ((value->ipRev != ipRev) && (value->portRev != portRev)) {
        goto TCP_REVERSE;
      } else {
        goto TCP_MISS;
      }

    TCP_FORWARD:;

      // Found in forward direction
      if (value->state == SYN_SENT) {
        // Still haven't received a SYN,ACK To the SYN
        if ((pkt->flags & TCPHDR_SYN) != 0 &&
            (pkt->flags | TCPHDR_SYN) == TCPHDR_SYN) {
          // Another SYN. It is valid, probably a retransmission.
          value->ttl = *timestamp + TCP_SYN_SENT;
          goto forward_action;
        } else {
          // Receiving packets outside the 3-Way handshake without completing
          // the handshake
          // TODO: Drop it?

          pcn_log(ctx, LOG_TRACE,
                  "[ConntrackTableUpdate] [FW_DIRECTION] Failed ACK check in "
                  "SYN_SENT state. Flags: %x",
                  pkt->flags);

          pkt->connStatus = INVALID;
          goto forward_action;
        }
      }

      if (value->state == SYN_RECV) {
        // Expecting an ACK here
        if ((pkt->flags & TCPHDR_ACK) != 0 &&
            (pkt->flags | TCPHDR_ACK) == TCPHDR_ACK &&
            (pkt->ackN == value->sequence)) {
          // Valid ACK to the SYN, ACK
          value->state = ESTABLISHED;
          value->ttl = *timestamp + TCP_ESTABLISHED;

          pcn_log(ctx, LOG_TRACE,
                  "[ConntrackTableUpdate] [FW_DIRECTION] Changing state from "
                  "SYN_RECV to ESTABLISHED");

          goto forward_action;
        } else {
          // Validation failed, either ACK is not the only flag set or the ack
          // number is wrong
          // TODO: drop it?

          pcn_log(ctx, LOG_TRACE,
                  "[ConntrackTableUpdate] [FW_DIRECTION] Failed ACK check in "
                  "SYN_RECV state. Flags: %x",
                  pkt->flags);

          pkt->connStatus = INVALID;
          goto forward_action;
        }
      }

      if (value->state == ESTABLISHED) {
        if ((pkt->flags & TCPHDR_FIN) != 0) {
          // Received first FIN from "original" direction.
          // Changing state to FIN_WAIT_1
          value->state = FIN_WAIT_1;
          value->ttl = *timestamp + TCP_FIN_WAIT;
          value->sequence = pkt->ackN;

          pcn_log(ctx, LOG_TRACE,
                  "[ConntrackTableUpdate] [FW_DIRECTION] Changing state from "
                  "ESTABLISHED to FIN_WAIT_1. Seq: %u",
                  value->sequence);

          goto forward_action;
        } else {
          value->ttl = *timestamp + TCP_ESTABLISHED;
          goto forward_action;
        }
      }

      if (value->state == FIN_WAIT_1) {
        // Received FIN in reverse direction, waiting for ack from this side
        if ((pkt->flags & TCPHDR_ACK) != 0 && (pkt->seqN == value->sequence)) {
          // Received ACK
          value->state = FIN_WAIT_2;
          value->ttl = *timestamp + TCP_FIN_WAIT;

          pcn_log(ctx, LOG_TRACE,
                  "[ConntrackTableUpdate] [FW_DIRECTION] Changing state from "
                  "FIN_WAIT_1 to FIN_WAIT_2");

        } else {
          // Validation failed, either ACK is not the only flag set or the ack
          // number is wrong
          // TODO: drop it?
          pcn_log(ctx, LOG_TRACE,
                  "[ConntrackTableUpdate] [FW_DIRECTION] Failed ACK check in "
                  "FIN_WAIT_1 state. Flags: %x. AckSeq: %u",
                  pkt->flags, pkt->ackN);

          pkt->connStatus = INVALID;
          goto forward_action;
        }
      }

      if (value->state == FIN_WAIT_2) {
        // Already received and acked FIN in rev direction, waiting the FIN from
        // the
        // this side
        if ((pkt->flags & TCPHDR_FIN) != 0) {
          // FIN received. Let's wait for it to be acknowledged.
          value->state = LAST_ACK;
          value->ttl = *timestamp + TCP_LAST_ACK;
          value->sequence = pkt->ackN;

          pcn_log(ctx, LOG_TRACE,
                  "[ConntrackTableUpdate] [FW_DIRECTION] Changing state from "
                  "FIN_WAIT_2 to LAST_ACK");
          goto forward_action;
        } else {
          // Still receiving packets

          pcn_log(ctx, LOG_TRACE,
                  "[ConntrackTableUpdate] [FW_DIRECTION] Failed FIN check in "
                  "FIN_WAIT_2 state. Flags: %x. Seq: %u",
                  pkt->flags, value->sequence);

          value->ttl = *timestamp + TCP_FIN_WAIT;
          goto forward_action;
        }
      }

      if (value->state == LAST_ACK) {
        if ((pkt->flags & TCPHDR_ACK && pkt->seqN == value->sequence) != 0) {
          // Ack to the last FIN.
          value->state = TIME_WAIT;
          value->ttl = *timestamp + TCP_LAST_ACK;

          pcn_log(ctx, LOG_TRACE,
                  "[ConntrackTableUpdate] [FW_DIRECTION] Changing state from "
                  "LAST_ACK to TIME_WAIT");

          goto forward_action;
        }
        // Still receiving packets
        value->ttl = *timestamp + TCP_LAST_ACK;
        goto forward_action;
      }

      if (value->state == TIME_WAIT) {
        if (pkt->connStatus == NEW) {
          goto TCP_MISS;
        } else {
          // Let the packet go, but do not update timers.
          goto forward_action;
        }
      }

      pcn_log(ctx, LOG_DEBUG,
              "[ConntrackTableUpdate] [FW_DIRECTION] Should not get here. "
              "Flags: %x. State: %d. ",
              pkt->flags, value->state);
      // TODO Unexpected situation
      goto forward_action;

    TCP_REVERSE:;

      // Found in reverse direction
      if (value->state == SYN_SENT) {
        // This should be a SYN, ACK answer
        if ((pkt->flags & TCPHDR_ACK) != 0 && (pkt->flags & TCPHDR_SYN) != 0 &&
            (pkt->flags | (TCPHDR_SYN | TCPHDR_ACK)) ==
                (TCPHDR_SYN | TCPHDR_ACK) &&
            pkt->ackN == value->sequence) {
          value->state = SYN_RECV;
          value->ttl = *timestamp + TCP_SYN_RECV;
          value->sequence = pkt->seqN + HEX_BE_ONE;

          pcn_log(ctx, LOG_TRACE,
                  "[ConntrackTableUpdate] [REV_DIRECTION] Changing state from "
                  "SYN_SENT to SYN_RECV");

          goto forward_action;
        }
        // Here is an unexpected packet, only a SYN, ACK is acepted as an answer
        // to a SYN
        // TODO: Drop it?
        pkt->connStatus = INVALID;
        goto forward_action;
      }

      if (value->state == SYN_RECV) {
        // The only acceptable packet in SYN_RECV here is a SYN,ACK
        // retransmission
        if ((pkt->flags & TCPHDR_ACK) != 0 && (pkt->flags & TCPHDR_SYN) != 0 &&
            (pkt->flags | (TCPHDR_SYN | TCPHDR_ACK)) ==
                (TCPHDR_SYN | TCPHDR_ACK) &&
            pkt->ackN == value->sequence) {
          value->ttl = *timestamp + TCP_SYN_RECV;
          goto forward_action;
        }
        pkt->connStatus = INVALID;
        goto forward_action;
      }

      if (value->state == ESTABLISHED) {
        if ((pkt->flags & TCPHDR_FIN) != 0) {
          // Initiating closing sequence
          value->state = FIN_WAIT_1;
          value->ttl = *timestamp + TCP_FIN_WAIT;
          value->sequence = pkt->ackN;

          pcn_log(ctx, LOG_TRACE,
                  "[ConntrackTableUpdate] [REV_DIRECTION] Changing state from "
                  "ESTABLISHED to FIN_WAIT_1. Seq: %x",
                  value->sequence);

          goto forward_action;
        } else {
          value->ttl = *timestamp + TCP_ESTABLISHED;
          goto forward_action;
        }
      }

      if (value->state == FIN_WAIT_1) {
        // Received FIN in reverse direction, waiting for ack from this side
        if ((pkt->flags & TCPHDR_ACK) != 0 && (pkt->seqN == value->sequence)) {
          // Received ACK
          value->state = FIN_WAIT_2;
          value->ttl = *timestamp + TCP_FIN_WAIT;

          pcn_log(ctx, LOG_TRACE,
                  "[ConntrackTableUpdate] [REV_DIRECTION] Changing state from "
                  "FIN_WAIT_1 to FIN_WAIT_2");

          // Don't forward packet, we can continue performing the check in case
          // the current packet is a ACK,FIN. In this case we match the next if
          // statement
        } else {
          // Validation failed, either ACK is not the only flag set or the ack
          // number is wrong
          // TODO: drop it?

          pcn_log(ctx, LOG_TRACE,
                  "[ConntrackTableUpdate] [REV_DIRECTION] Failed ACK check in "
                  "FIN_WAIT_1 state. Flags: %d. AckSeq: %d",
                  pkt->flags, pkt->ackN);

          pkt->connStatus = INVALID;
          goto forward_action;
        }
      }

      if (value->state == FIN_WAIT_2) {
        // Already received and acked FIN in "original" direction, waiting the
        // FIN from
        // this side
        if ((pkt->flags & TCPHDR_FIN) != 0) {
          // FIN received. Let's wait for it to be acknowledged.
          value->state = LAST_ACK;
          value->ttl = *timestamp + TCP_LAST_ACK;
          value->sequence = pkt->ackN;

          pcn_log(ctx, LOG_TRACE,
                  "[ConntrackTableUpdate] [REV_DIRECTION] Changing state from "
                  "FIN_WAIT_1 to LAST_ACK");

          goto forward_action;
        } else {
          // Still receiving packets

          pcn_log(ctx, LOG_TRACE,
                  "[ConntrackTableUpdate] [REV_DIRECTION] Failed FIN check in "
                  "FIN_WAIT_2 state. Flags: %d. Seq: %d",
                  pkt->flags, value->sequence);

          value->ttl = *timestamp + TCP_FIN_WAIT;
          goto forward_action;
        }
      }

      if (value->state == LAST_ACK) {
        if ((pkt->flags & TCPHDR_ACK && pkt->seqN == value->sequence) != 0) {
          // Ack to the last FIN.
          value->state = TIME_WAIT;
          value->ttl = *timestamp + TCP_LAST_ACK;

          pcn_log(ctx, LOG_TRACE,
                  "[ConntrackTableUpdate] [REV_DIRECTION] Changing state from "
                  "LAST_ACK to TIME_WAIT");

          goto forward_action;
        }
        // Still receiving packets
        value->ttl = *timestamp + TCP_LAST_ACK;
        goto forward_action;
      }

      if (value->state == TIME_WAIT) {
        if (pkt->connStatus == NEW) {
          goto TCP_MISS;
        } else {
          // Let the packet go, but do not update timers.
          goto forward_action;
        }
      }

      pcn_log(ctx, LOG_DEBUG,
              "[ConntrackTableUpdate] [REV_DIRECTION] Should not get here. "
              "Flags: %d. "
              "State: %d. ",
              pkt->flags, value->state);
      goto forward_action;
    }

  TCP_MISS:;

    // New entry. It has to be a SYN.
    if ((pkt->flags & TCPHDR_SYN) != 0 &&
        (pkt->flags | TCPHDR_SYN) == TCPHDR_SYN) {
      newEntry.state = SYN_SENT;
      newEntry.ttl = *timestamp + TCP_SYN_SENT;
      newEntry.sequence = pkt->seqN + HEX_BE_ONE;

      newEntry.ipRev = ipRev;
      newEntry.portRev = portRev;

      connections.update(&key, &newEntry);
      goto forward_action;
    } else {
      // Validation failed
      pcn_log(ctx, LOG_DEBUG, "Validation failed %d", pkt->flags);
      goto forward_action;
    }
  }

  /* == UDP == */
  if (pkt->l4proto == IPPROTO_UDP) {
    value = connections.lookup(&key);
    if (value != NULL) {
      if ((value->ipRev == ipRev) && (value->portRev == portRev)) {
        goto UDP_FORWARD;
      } else if ((value->ipRev != ipRev) && (value->portRev != portRev)) {
        goto UDP_REVERSE;
      } else {
        goto UDP_MISS;
      }

    UDP_FORWARD:;

      // Valid entry
      if (value->state == NEW) {
        // An entry was already present with the NEW state. This means that
        // there has been no answer, from the other side. Connection is still
        // NEW.
        // TODO: For now I am refreshing the TTL, this can lead to an DoS
        // attack where the attacker prevents the entry from being deleted by
        // continuosly sending packets.
        value->ttl = *timestamp + UDP_NEW_TIMEOUT;
        goto forward_action;
      } else {
        // value->state == ESTABLISHED
        value->ttl = *timestamp + UDP_ESTABLISHED_TIMEOUT;
        goto forward_action;
      }

    UDP_REVERSE:;

      if (value->state == NEW) {
        // An entry was present in the rev direction with the NEW state. This
        // means that this is an answer, from the other side. Connection is
        // now ESTABLISHED.
        value->ttl = *timestamp + UDP_NEW_TIMEOUT;
        value->state = ESTABLISHED;

        pcn_log(ctx, LOG_TRACE,
                "[ConntrackTableUpdate] [REV_DIRECTION] Changing state from "
                "NEW to ESTABLISHED");

        goto forward_action;
      } else {
        // value->state == ESTABLISHED
        value->ttl = *timestamp + UDP_ESTABLISHED_TIMEOUT;
        goto forward_action;
      }
    }

  UDP_MISS:;

    // No entry found in both directions. Create one.
    newEntry.ttl = *timestamp + UDP_NEW_TIMEOUT;
    newEntry.state = NEW;
    newEntry.sequence = 0;

    newEntry.ipRev = ipRev;
    newEntry.portRev = portRev;

    connections.insert(&key, &newEntry);
    goto forward_action;
  }

  /* == ICMP  == */
  if (pkt->l4proto == IPPROTO_ICMP) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    if (data + 34 + sizeof(struct icmphdr) > data_end)
      return RX_DROP;
    struct icmphdr *icmp = data + 34;
    if (icmp->type == ICMP_ECHO) {
      // Echo request is always treated as the first of the connection
      newEntry.ttl = *timestamp + ICMP_TIMEOUT;
      newEntry.state = NEW;
      newEntry.sequence = 0;

      newEntry.ipRev = ipRev;
      newEntry.portRev = portRev;

      connections.insert(&key, &newEntry);
      goto forward_action;
    }

    if (icmp->type == ICMP_ECHOREPLY) {
      // No more packets expected here.
      connections.delete(&key);
      goto forward_action;
    }

    // All other ICMPs are not supported or RELATED (so nothing to do)
    // TODO: ICMP Related packet for now are not refreshing TTL. Is that right?

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

    pcn_log(
        ctx, LOG_TRACE,
        "Conntrack Table Update +bpf_fib_lookup+ redirect pkt to ifindex %d ",
        fib_params.ifindex);
    return tx_port.redirect_map(fib_params.ifindex, 0);

  } else {
    pcn_log(ctx, LOG_TRACE,
            "Conntrack Table Update INGRESS: ACCEPT packet, after save in "
            "session table");
    return RX_OK;
  }
#endif

#if _EGRESS_LOGIC
  pcn_log(ctx, LOG_TRACE,
          "Conntrack Table Update EGRESS: ACCEPT packet, after save in session "
          "table");
  return RX_OK;
#endif

#else
  pcn_log(ctx, LOG_TRACE,
          "Conntrack Table Update: ACCEPT packet, after save in session table");
  return RX_OK;
#endif

#endif
}
