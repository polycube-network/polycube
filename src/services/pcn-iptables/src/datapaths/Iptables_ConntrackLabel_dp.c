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

#define SESSION_DIM _SESSION_DIM

#define AF_INET 2 /* Internet IP Protocol 	*/

/* MACRO */

#define BIT_SET(y, mask) (y |= (mask))
#define BIT_CLEAR(y, mask) (y &= ~(mask))

#define CHECK_MASK_IS_SET(y, mask) (y & mask)

/* ENUM */

enum {
  INPUT_LABELING,    // goto input chain and label packet
  FORWARD_LABELING,  // goto forward chain and label packet
  OUTPUT_LABELING,   // goto output chain and label packet
  PASS_LABELING,     // one chain is hit (IN/PUT/FWD) but there are no rules and
                     // default action is accept. Label packet and let it pass.
  PASS_NO_LABELING,  // OUTPUT chain is not hit, let the packet pass without
                     // labeling //NEVER HIT
  DROP_NO_LABELING   // one chain is hit (IN/PUT/FWD) but there are no rules and
                     // default action is DROP. //NEVER HIT
};

// connection tracking states
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

// session table direction
enum {
  DIRECTION_FORWARD,  // Forward direction in session table
  DIRECTION_REVERSE   // Reverse direction in session table
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

// session table value
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
BPF_TABLE("extern", int, int, forwardingDecision, 1);
BPF_TABLE("extern", uint32_t, struct session_v, session, SESSION_DIM);

#if _INGRESS_LOGIC
BPF_TABLE_SHARED("percpu_array", int, uint64_t, timestamp, 1);

BPF_TABLE_SHARED("percpu_array", int, u64, pkts_acceptestablished_Input, 1);
BPF_TABLE_SHARED("percpu_array", int, u64, bytes_acceptestablished_Input, 1);

BPF_TABLE_SHARED("percpu_array", int, u64, pkts_acceptestablished_Forward, 1);
BPF_TABLE_SHARED("percpu_array", int, u64, bytes_acceptestablished_Forward, 1);
#endif

#if _EGRESS_LOGIC
BPF_TABLE("extern", int, uint64_t, timestamp, 1);

BPF_TABLE_SHARED("percpu_array", int, u64, pkts_acceptestablished_Output, 1);
BPF_TABLE_SHARED("percpu_array", int, u64, bytes_acceptestablished_Output, 1);
#endif

/* INLINE FUNCTIONS */

static __always_inline int *getForwardingDecision() {
  int key = 0;
  return forwardingDecision.lookup(&key);
}

static __always_inline uint64_t *time_get_ns() {
  int key = 0;
  return timestamp.lookup(&key);
}

#if _INGRESS_LOGIC
static __always_inline void incrementAcceptEstablishedInput(u32 bytes) {
  u64 *value;
  int zero = 0;
  value = pkts_acceptestablished_Input.lookup(&zero);
  if (value) {
    *value += 1;
  }

  value = bytes_acceptestablished_Input.lookup(&zero);
  if (value) {
    *value += bytes;
  }
}

static __always_inline void incrementAcceptEstablishedForward(u32 bytes) {
  u64 *value;
  int zero = 0;
  value = pkts_acceptestablished_Forward.lookup(&zero);
  if (value) {
    *value += 1;
  }

  value = bytes_acceptestablished_Forward.lookup(&zero);
  if (value) {
    *value += bytes;
  }
}
#endif

#if _EGRESS_LOGIC
static __always_inline void incrementAcceptEstablishedOutput(u32 bytes) {
  u64 *value;
  int zero = 0;
  value = pkts_acceptestablished_Output.lookup(&zero);
  if (value) {
    *value += 1;
  }

  value = bytes_acceptestablished_Output.lookup(&zero);
  if (value) {
    *value += bytes;
  }
}
#endif

/* BPF PROGRAM */

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  pcn_log(ctx, LOG_DEBUG, "[_HOOK] [ConntrackLabel] received packet");
  struct packetHeaders *pkt;
  int k = 0;
  pkt = packet.lookup(&k);

  if (pkt == NULL) {
    // Not possible
    return RX_DROP;
  }

  struct session_v *session_value_p = 0;
  uint32_t sessionId = pkt->sessionId;

  session_value_p = session.lookup(&sessionId);
  if (session_value_p == NULL) {
    pcn_log(ctx, LOG_DEBUG,
            "[_HOOK] [ConntrackLabel] lookup sessionId: %d FAILED ", sessionId);
    return RX_DROP;
  }

  uint64_t *timestamp;
  timestamp = time_get_ns();

  if (timestamp == NULL) {
    pcn_log(ctx, LOG_DEBUG,
            "[_HOOK] [ConntrackTableUpdate] timestamp NULL. ret DROP ");
    return RX_DROP;
  }

  pkt->mask = 0;

  /* == TCP  == */
  if (pkt->l4proto == IPPROTO_TCP) {
    pcn_log(
        ctx, LOG_DEBUG,
        "[_HOOK] [ConntrackLabel] [TCP] session->setMask: 0x%x direction %d ",
        session_value_p->setMask, pkt->direction);
    if (CHECK_MASK_IS_SET(session_value_p->setMask, BIT(BIT_CONNTRACK))) {
      if (pkt->direction == DIRECTION_FORWARD) {
        goto TCP_FORWARD;
      } else if (pkt->direction == DIRECTION_REVERSE) {
        goto TCP_REVERSE;
      }
    } else {
      goto TCP_MISS;
    }

  TCP_FORWARD:;

    pcn_log(ctx, LOG_DEBUG, "[_HOOK] [ConntrackLabel] [TCP] [FWD] ");

    // If it is a RST, label it as established.
    if ((pkt->flags & TCPHDR_RST) != 0) {
      pkt->connStatus = ESTABLISHED;
      pcn_log(ctx, LOG_DEBUG,
              "[_HOOK] [ConntrackLabel] [TCP] [FWD] state [%d] -> [%d] label "
              "[%d] RST -> keep ESTABLISHED ",
              session_value_p->state, session_value_p->state, pkt->connStatus);
      goto action;
    }

    if (session_value_p->state == SYN_SENT) {
      // Still haven't received a SYN,ACK To the SYN
      if ((pkt->flags & TCPHDR_SYN) != 0 &&
          (pkt->flags | TCPHDR_SYN) == TCPHDR_SYN) {
        // Another SYN. It is valid, probably a retransmission.
        // connections.delete(&key);
        pkt->connStatus = NEW;
        pkt->ttl = *timestamp + TCP_SYN_SENT;
        BIT_SET(pkt->mask, BIT(BIT_CT_SET_TTL));
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [FWD] state [%d] -> [%d] label "
                "[%d] [SYN] -> keep NEW ",
                session_value_p->state, session_value_p->state,
                pkt->connStatus);
        goto action;
      } else {
        // Receiving packets outside the 3-Way handshake without completing
        // the handshake
        // TODO: Drop it?
        pkt->connStatus = INVALID;
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [FWD] state [%d] -> [%d] label "
                "[%d] [!SYN] INVALID",
                session_value_p->state, session_value_p->state,
                pkt->connStatus);
        goto action;
      }
    }
    if (session_value_p->state == SYN_RECV) {
      // Expecting an ACK here
      if ((pkt->flags & TCPHDR_ACK) != 0 &&
          (pkt->flags | TCPHDR_ACK) == TCPHDR_ACK &&
          (pkt->ackN == session_value_p->sequence)) {
        // Valid ACK to the SYN, ACK
        pkt->connStatus = ESTABLISHED;
        pkt->state = ESTABLISHED;
        pkt->ttl = *timestamp + TCP_ESTABLISHED;
        BIT_SET(pkt->mask, BIT(BIT_CT_SET_TTL) | BIT(BIT_CT_SET_STATE));
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [FWD] state [%d] -> [%d] label "
                "[%d] [ACK && valid reply to SYN,ACK] ",
                session_value_p->state, pkt->state, pkt->connStatus);
        goto action;
      } else {
        // Validation failed, either ACK is not the only flag set or the ack
        // number is wrong
        // TODO: drop it?
        pkt->connStatus = INVALID;
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [FWD] state [%d] -> [%d] label "
                "[%d] ![ACK || valid reply to SYN,ACK] INVALID",
                session_value_p->state, session_value_p->state,
                pkt->connStatus);
        goto action;
      }
    }

    if (session_value_p->state == ESTABLISHED) {
      pkt->connStatus = ESTABLISHED;

      if ((pkt->flags & TCPHDR_FIN) != 0) {
        // Received first FIN from "original" direction.
        // Changing state to FIN_WAIT_1
        pkt->state = FIN_WAIT_1;
        pkt->ttl = *timestamp + TCP_FIN_WAIT;
        pkt->sequence = pkt->ackN;
        BIT_SET(pkt->mask, BIT(BIT_CT_SET_STATE) | BIT(BIT_CT_SET_TTL) |
                                  BIT(BIT_CT_SET_SEQUENCE));
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [FWD] state [%d] -> [%d] label "
                "[%d] [FIN]",
                session_value_p->state, pkt->state, pkt->connStatus);
        goto action;
      } else {
        pkt->ttl = *timestamp + TCP_ESTABLISHED;
        BIT_SET(pkt->mask, BIT(BIT_CT_SET_TTL));
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [FWD] state [%d] -> [%d] label "
                "[%d] ![FIN]",
                session_value_p->state, session_value_p->state,
                pkt->connStatus);
        goto action;
      }
    }

    if (session_value_p->state == FIN_WAIT_1) {
      pkt->connStatus = ESTABLISHED;
      // Received FIN in reverse direction, waiting for ack from this side
      if ((pkt->flags & TCPHDR_ACK) != 0 &&
          (pkt->seqN == session_value_p->sequence)) {
        // Received ACK
        pkt->state = FIN_WAIT_2;
        pkt->ttl = *timestamp + TCP_FIN_WAIT;
        BIT_SET(pkt->mask, BIT(BIT_CT_SET_STATE) | BIT(BIT_CT_SET_TTL));
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [FWD] state [%d] -> [%d] label "
                "[%d] [ACK && seq]",
                session_value_p->state, pkt->state, pkt->connStatus);

      } else {
        // Validation failed, either ACK is not the only flag set or the ack
        // number is wrong
        // TODO: drop it?
        pkt->connStatus = INVALID;
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [FWD] state [%d] -> [%d] label "
                "[%d] ![ACK || seq] INVALID",
                session_value_p->state, session_value_p->state,
                pkt->connStatus);
        goto action;
      }
    }

    if (session_value_p->state == FIN_WAIT_2) {
      pkt->connStatus = ESTABLISHED;
      // Already received and acked FIN in rev direction, waiting the FIN from
      // the
      // this side
      if ((pkt->flags & TCPHDR_FIN) != 0) {
        // FIN received. Let's wait for it to be acknowledged.
        pkt->state = LAST_ACK;
        pkt->ttl = *timestamp + TCP_LAST_ACK;
        pkt->sequence = pkt->ackN;
        BIT_SET(pkt->mask, BIT(BIT_CT_SET_STATE) | BIT(BIT_CT_SET_TTL) |
                                  BIT(BIT_CT_SET_SEQUENCE));
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [FWD] state [%d] -> [%d] label "
                "[%d] [FIN]",
                session_value_p->state, pkt->state, pkt->connStatus);
        goto action;
      } else {
        // Still receiving packets
        pkt->ttl = *timestamp + TCP_FIN_WAIT;
        BIT_SET(pkt->mask, BIT(BIT_CT_SET_TTL));
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [FWD] state [%d] -> [%d] label "
                "[%d] [!FIN]",
                session_value_p->state, session_value_p->state,
                pkt->connStatus);
        goto action;
      }
    }

    if (session_value_p->state == LAST_ACK) {
      pkt->connStatus = ESTABLISHED;
      if ((pkt->flags & TCPHDR_ACK && pkt->seqN == session_value_p->sequence) !=
          0) {
        // Ack to the last FIN.
        pkt->state = TIME_WAIT;
        pkt->ttl = *timestamp + TCP_LAST_ACK;
        BIT_SET(pkt->mask, BIT(BIT_CT_SET_STATE) | BIT(BIT_CT_SET_TTL));
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [FWD] state [%d] -> [%d] label "
                "[%d] ",
                session_value_p->state, pkt->state, pkt->connStatus);
        goto action;
      }
      // Still receiving packets
      pkt->ttl = *timestamp + TCP_LAST_ACK;
      BIT_SET(pkt->mask, BIT(BIT_CT_SET_TTL));
      pcn_log(
          ctx, LOG_DEBUG,
          "[_HOOK] [ConntrackLabel] [TCP] [FWD] state [%d] -> [%d] label [%d] ",
          session_value_p->state, session_value_p->state, pkt->connStatus);
      goto action;
    }

    if (session_value_p->state == TIME_WAIT) {
      // If the state is TIME_WAIT but we receive a new SYN the connection is
      // considered NEW
      if ((pkt->flags & TCPHDR_SYN) != 0 &&
          (pkt->flags | TCPHDR_SYN) == TCPHDR_SYN) {
        pkt->connStatus = NEW;
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [FWD] state [%d] -> [%d] label "
                "[%d] [TIME_WAIT] [SYN] GOTO TCP_MISS",
                session_value_p->state, session_value_p->state,
                pkt->connStatus);
        goto TCP_MISS;
      } else {
        pkt->connStatus = INVALID;  // ADDED
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [FWD] state [%d] -> [%d] label "
                "[%d] [TIME_WAIT] [!SYN] ",
                session_value_p->state, session_value_p->state,
                pkt->connStatus);
        goto action;
      }
    }

    pkt->connStatus = INVALID;
    pcn_log(ctx, LOG_DEBUG,
            "[_HOOK] [ConntrackLabel] [TCP] [FWD] state [%d] -> [%d] label "
            "[%d] Unexpected ",
            session_value_p->state, session_value_p->state, pkt->connStatus);

    goto action;

  TCP_REVERSE:;

    pcn_log(ctx, LOG_DEBUG, "[_HOOK] [ConntrackLabel] [TCP] [REV] ");

    // If it is a RST, label it as established.
    if ((pkt->flags & TCPHDR_RST) != 0) {
      pkt->connStatus = ESTABLISHED;
      pcn_log(ctx, LOG_DEBUG,
              "[_HOOK] [ConntrackLabel] [TCP] [REV] state [%d] -> [%d] label "
              "[%d] RST",
              session_value_p->state, session_value_p->state, pkt->connStatus);
      goto action;
    }

    if (session_value_p->state == SYN_SENT) {
      // This should be a SYN, ACK answer
      if ((pkt->flags & TCPHDR_ACK) != 0 && (pkt->flags & TCPHDR_SYN) != 0 &&
          (pkt->flags | (TCPHDR_SYN | TCPHDR_ACK)) ==
              (TCPHDR_SYN | TCPHDR_ACK) &&
          pkt->ackN == session_value_p->sequence) {
        pkt->state = SYN_RECV;
        pkt->ttl = *timestamp + TCP_SYN_RECV;
        pkt->sequence = pkt->seqN + HEX_BE_ONE;
        BIT_SET(pkt->mask, BIT(BIT_CT_SET_STATE) | BIT(BIT_CT_SET_TTL) |
                                  BIT(BIT_CT_SET_SEQUENCE));
        pkt->connStatus = ESTABLISHED;
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [REV] state [%d] -> [%d] label "
                "[%d] [SYN,ACK in sequence Answer]",
                session_value_p->state, session_value_p->state,
                pkt->connStatus);
        goto action;
      }
      // Here is an unexpected packet, only a SYN, ACK is acepted as an answer
      // to a SYN
      // TODO: Drop it?
      pkt->connStatus = INVALID;
      pcn_log(ctx, LOG_DEBUG,
              "[_HOOK] [ConntrackLabel] [TCP] [REV] state [%d] -> [%d] label "
              "[%d] ![SYN,ACK in sequence]",
              session_value_p->state, pkt->state, pkt->connStatus);
      goto action;
    }

    if (session_value_p->state == SYN_RECV) {
      // The only acceptable packet in SYN_RECV here is a SYN,ACK
      // retransmission
      if ((pkt->flags & TCPHDR_ACK) != 0 && (pkt->flags & TCPHDR_SYN) != 0 &&
          (pkt->flags | (TCPHDR_SYN | TCPHDR_ACK)) ==
              (TCPHDR_SYN | TCPHDR_ACK) &&
          pkt->ackN == session_value_p->sequence) {
        pkt->ttl = *timestamp + TCP_SYN_RECV;
        BIT_SET(pkt->mask, BIT(BIT_CT_SET_TTL));
        pkt->connStatus = ESTABLISHED;
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [REV] state [%d] -> [%d] label "
                "[%d] [SYN,ACK retransmission]",
                session_value_p->state, session_value_p->state,
                pkt->connStatus);
        goto action;
      }
      pkt->connStatus = INVALID;
      pcn_log(ctx, LOG_DEBUG,
              "[_HOOK] [ConntrackLabel] [TCP] [REV] state [%d] -> [%d] label "
              "[%d] ![SYN,ACK retransmission]",
              session_value_p->state, session_value_p->state, pkt->connStatus);
      goto action;
    }

    if (session_value_p->state == ESTABLISHED) {
      pkt->connStatus = ESTABLISHED;

      if ((pkt->flags & TCPHDR_FIN) != 0) {
        // Initiating closing sequence
        pkt->state = FIN_WAIT_1;
        pkt->ttl = *timestamp + TCP_FIN_WAIT;
        pkt->sequence = pkt->ackN;

        BIT_SET(pkt->mask, BIT(BIT_CT_SET_STATE) | BIT(BIT_CT_SET_TTL) |
                                  BIT(BIT_CT_SET_SEQUENCE));
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [REV] state [%d] -> [%d] label "
                "[%d] [FIN]",
                session_value_p->state, pkt->state, pkt->connStatus);
        goto action;
      } else {
        pkt->ttl = *timestamp + TCP_ESTABLISHED;
        BIT_SET(pkt->mask, BIT(BIT_CT_SET_TTL));
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [REV] state [%d] -> [%d] label "
                "[%d] ![FIN]",
                session_value_p->state, session_value_p->state,
                pkt->connStatus);
        goto action;
      }
    }

    if (session_value_p->state == FIN_WAIT_1) {
      pkt->connStatus = ESTABLISHED;

      // Received FIN in reverse direction, waiting for ack from this side
      if ((pkt->flags & TCPHDR_ACK) != 0 &&
          (pkt->seqN == session_value_p->sequence)) {
        // Received ACK
        pkt->state = FIN_WAIT_2;
        pkt->ttl = *timestamp + TCP_FIN_WAIT;
        BIT_SET(pkt->mask, BIT(BIT_CT_SET_STATE) | BIT(BIT_CT_SET_TTL));
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [REV] state [%d] -> [%d] label "
                "[%d] [ACK (response to FIN)]",
                session_value_p->state, pkt->state, pkt->connStatus);

        // Don't forward packet, we can continue performing the check in case
        // the current packet is a ACK,FIN. In this case we match the next if
        // statement
      } else {
        // Validation failed, either ACK is not the only flag set or the ack
        // number is wrong
        pkt->connStatus = INVALID;
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [REV] state [%d] -> [%d] label "
                "[%d] ![ACK (response to FIN)]",
                session_value_p->state, session_value_p->state,
                pkt->connStatus);

        goto action;
      }
    }

    if (session_value_p->state == FIN_WAIT_2) {
      pkt->connStatus = ESTABLISHED;

      // Already received and acked FIN in "original" direction, waiting the
      // FIN from
      // this side
      if ((pkt->flags & TCPHDR_FIN) != 0) {
        // FIN received. Let's wait for it to be acknowledged.
        pkt->state = LAST_ACK;
        pkt->ttl = *timestamp + TCP_LAST_ACK;
        pkt->sequence = pkt->ackN;
        BIT_SET(pkt->mask, BIT(BIT_CT_SET_STATE) | BIT(BIT_CT_SET_TTL) |
                                  BIT(BIT_CT_SET_SEQUENCE));
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [REV] state [%d] -> [%d] label "
                "[%d] [FIN]",
                session_value_p->state, pkt->state, pkt->connStatus);
        goto action;
      } else {
        // Still receiving packets
        pkt->ttl = *timestamp + TCP_FIN_WAIT;
        BIT_SET(pkt->mask, BIT(BIT_CT_SET_TTL));
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [REV] state [%d] -> [%d] label "
                "[%d] ![FIN]",
                session_value_p->state, session_value_p->state,
                pkt->connStatus);
        goto action;
      }
    }

    if (session_value_p->state == LAST_ACK) {
      pkt->connStatus = ESTABLISHED;

      if ((pkt->flags & TCPHDR_ACK && pkt->seqN == session_value_p->sequence) !=
          0) {
        // Ack to the last FIN.
        pkt->state = TIME_WAIT;
        pkt->ttl = *timestamp + TCP_LAST_ACK;
        BIT_SET(pkt->mask, BIT(BIT_CT_SET_STATE) | BIT(BIT_CT_SET_TTL));
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [REV] state [%d] -> [%d] label "
                "[%d] [ACK in seq]",
                session_value_p->state, pkt->state, pkt->connStatus);
        goto action;
      }
      // Still receiving packets
      pkt->ttl = *timestamp + TCP_LAST_ACK;
      BIT_SET(pkt->mask, BIT(BIT_CT_SET_TTL));
      pcn_log(ctx, LOG_DEBUG,
              "[_HOOK] [ConntrackLabel] [TCP] [REV] state [%d] -> [%d] label "
              "[%d] ![ACK in seq]",
              session_value_p->state, session_value_p->state, pkt->connStatus);
      goto action;
    }

    if (session_value_p->state == TIME_WAIT) {
      // If the state is TIME_WAIT but we receive a new SYN the connection is
      // considered NEW
      if ((pkt->flags & TCPHDR_SYN) != 0 &&
          (pkt->flags | TCPHDR_SYN) == TCPHDR_SYN) {
        pkt->connStatus = NEW;
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [REV] state [%d] -> [%d] label "
                "[%d] [SYN] goto TCP_MISS",
                session_value_p->state, session_value_p->state,
                pkt->connStatus);
        // TODO this is not working, we should exchange forward and reverse keys
        // in tupletosession table.
        goto TCP_MISS;
      } else {
        pkt->connStatus = INVALID;
        pcn_log(ctx, LOG_DEBUG,
                "[_HOOK] [ConntrackLabel] [TCP] [REV] state [%d] -> [%d] label "
                "[%d] ![SYN]",
                session_value_p->state, session_value_p->state,
                pkt->connStatus);
        goto action;
      }
    }

    pkt->connStatus = INVALID;
    pcn_log(ctx, LOG_DEBUG,
            "[_HOOK] [ConntrackLabel] [TCP] [REV] state [%d] -> [%d] label "
            "[%d] Exception invalid",
            session_value_p->state, session_value_p->state, pkt->connStatus);
    goto action;

  TCP_MISS:;

    pcn_log(ctx, LOG_DEBUG, "[_HOOK] [ConntrackLabel] [TCP] [MISS] ");

    // New entry. It has to be a SYN.
    if ((pkt->flags & TCPHDR_SYN) != 0 &&
        (pkt->flags | TCPHDR_SYN) == TCPHDR_SYN) {
      pkt->connStatus = NEW;

      pkt->state = SYN_SENT;
      pkt->ttl = *timestamp + TCP_SYN_SENT;
      pkt->sequence = pkt->seqN + HEX_BE_ONE;
      // TODO
      pkt->setMask = BIT(BIT_CONNTRACK);
      //      BIT_SET(pkt->setMask, BIT(BIT_CONNTRACK));
      BIT_SET(pkt->mask, BIT(BIT_CT_SET_STATE) | BIT(BIT_CT_SET_TTL) |
                                BIT(BIT_CT_SET_SEQUENCE) |
                                BIT(BIT_CT_SET_MASK));
      pcn_log(ctx, LOG_DEBUG,
              "[_HOOK] [ConntrackLabel] [TCP] [MISS] state [%d] -> [%d] label "
              "[%d] [SYN]",
              session_value_p->state, pkt->state, pkt->connStatus);

      goto action;
    } else {
      // Validation failed
      // TODO: drop it?
      pkt->connStatus = INVALID;
      pcn_log(ctx, LOG_DEBUG,
              "[_HOOK] [ConntrackLabel] [TCP] [MISS] state [%d] -> [%d] label "
              "[%d] ![SYN]",
              session_value_p->state, session_value_p->state, pkt->connStatus);
      goto action;
    }
  }

  /* == UDP == */
  if (pkt->l4proto == IPPROTO_UDP) {
    if (CHECK_MASK_IS_SET(session_value_p->setMask, BIT(BIT_CONNTRACK))) {
      if (pkt->direction == DIRECTION_FORWARD) {
        goto UDP_FORWARD;
      }
      if (pkt->direction == DIRECTION_REVERSE) {
        goto UDP_REVERSE;
      }
    } else {
      goto UDP_MISS;
    }

  UDP_FORWARD:;

    // Valid entry
    if (session_value_p->state == NEW) {
      // An entry was already present with the NEW state. This means that
      // there has been no answer, from the other side. Connection is still
      // NEW.
      pkt->connStatus = NEW;
      pkt->ttl = *timestamp + UDP_NEW_TIMEOUT;
      BIT_SET(pkt->mask, BIT(BIT_CT_SET_TTL));
      pcn_log(ctx, LOG_DEBUG,
              "[_HOOK] [ConntrackLabel] [UDP] [FWD] state [%d] -> [%d] label "
              "[%d] [NEW] ",
              session_value_p->state, session_value_p->state, pkt->connStatus);
      goto action;
    } else {
      pkt->ttl = *timestamp + UDP_ESTABLISHED_TIMEOUT;
      BIT_SET(pkt->mask, BIT(BIT_CT_SET_TTL));
      pkt->connStatus = ESTABLISHED;
      pcn_log(ctx, LOG_DEBUG,
              "[_HOOK] [ConntrackLabel] [UDP] [FWD] state [%d] -> [%d] label "
              "[%d] ![NEW]",
              session_value_p->state, session_value_p->state, pkt->connStatus);
      goto action;
    }

  UDP_REVERSE:;

    if (session_value_p->state == NEW) {
      // An entry was present in the rev direction with the NEW state. This
      // means that this is an answer, from the other side. Connection is
      // now ESTABLISHED.
      pkt->ttl = *timestamp + UDP_NEW_TIMEOUT;
      pkt->state = *timestamp + ESTABLISHED;
      BIT_SET(pkt->mask, BIT(BIT_CT_SET_TTL) | BIT(BIT_CT_SET_STATE));
      pkt->connStatus = ESTABLISHED;
      pcn_log(
          ctx, LOG_DEBUG,
          "[_HOOK] [ConntrackLabel] [UDP] [REV] state [%d] -> [%d] label [%d] ",
          session_value_p->state, pkt->state, pkt->connStatus);
      goto action;
    } else {
      pkt->ttl = *timestamp + UDP_ESTABLISHED_TIMEOUT;
      BIT_SET(pkt->mask, BIT(BIT_CT_SET_TTL));
      pkt->connStatus = ESTABLISHED;
      pcn_log(
          ctx, LOG_DEBUG,
          "[_HOOK] [ConntrackLabel] [UDP] [REV] state [%d] -> [%d] label [%d] ",
          session_value_p->state, pkt->state, pkt->connStatus);
      goto action;
    }

  UDP_MISS:;

    // No entry found in both directions. Create one.
    pkt->state = NEW;
    pkt->ttl = *timestamp + UDP_NEW_TIMEOUT;
    pkt->sequence = 0;
    //    BIT_SET(pkt->setMask, BIT(BIT_CONNTRACK));
    pkt->setMask = BIT(BIT_CONNTRACK);
    BIT_SET(pkt->mask, BIT(BIT_CT_SET_STATE) | BIT(BIT_CT_SET_TTL) |
                              BIT(BIT_CT_SET_SEQUENCE) | BIT(BIT_CT_SET_MASK));
    pkt->connStatus = NEW;
    pcn_log(
        ctx, LOG_DEBUG,
        "[_HOOK] [ConntrackLabel] [UDP] [MISS] state [%d] -> [%d] label [%d] ",
        session_value_p->state, pkt->state, pkt->connStatus);
    goto action;
  }

  /* == ICMP  == */
  if (pkt->l4proto == IPPROTO_ICMP) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    // 34 = sizeof(eth_hdr) + sizeof(ip_hdr)
    if (data + 34 + sizeof(struct icmphdr) > data_end) {
      return RX_DROP;
    }
    struct icmphdr *icmp = data + 34;

    if ((icmp->type == ICMP_ECHO) || (icmp->type == ICMP_ECHOREPLY)) {
      pcn_log(ctx, LOG_DEBUG,
              "[_HOOK] [ConntrackLabel] [ICMP] [ECHO/ECHOREPLY]");

      if (CHECK_MASK_IS_SET(session_value_p->setMask, BIT(BIT_CONNTRACK))) {
        // hit, entry already present
        if (pkt->direction ==
            DIRECTION_FORWARD) {  //} /*&& (icmp->type == ICMP_ECHO)*/) {
          pkt->connStatus = session_value_p->state;
          pkt->ttl = *timestamp + ICMP_TIMEOUT;
          pkt->sequence = icmp->un.echo.sequence;
          BIT_SET(pkt->mask, BIT(BIT_CT_SET_TTL) | BIT(BIT_CT_SET_SEQUENCE));
          pcn_log(ctx, LOG_DEBUG,
                  "[_HOOK] [ConntrackLabel] [ICMP] [FWD] & [ECHO] state [%d] "
                  "-> [%d] label [%d] ",
                  session_value_p->state, session_value_p->state,
                  pkt->connStatus);
          goto action;
        }
        if (pkt->direction ==
            DIRECTION_REVERSE) {  //&& (icmp->type == ICMP_ECHOREPLY)) {
          if (session_value_p->state == NEW) {
            pkt->state = ESTABLISHED;
            pkt->sequence = icmp->un.echo.sequence;
            BIT_SET(pkt->mask,
                    BIT(BIT_CT_SET_STATE) | BIT(BIT_CT_SET_SEQUENCE));
            pcn_log(ctx, LOG_DEBUG,
                    "[_HOOK] [ConntrackLabel] [ICMP] [REV] & [REPLY] state "
                    "[%d] -> [%d] label [%d] ",
                    session_value_p->state, pkt->state, pkt->connStatus);
          }
          pkt->connStatus = ESTABLISHED;
          pkt->ttl = *timestamp + ICMP_TIMEOUT;
          BIT_SET(pkt->mask, BIT(BIT_CT_SET_TTL));
          pcn_log(ctx, LOG_DEBUG,
                  "[_HOOK] [ConntrackLabel] [ICMP] [REV] & [REPLY] state [%d] "
                  "-> [%d] label [%d] ",
                  session_value_p->state, session_value_p->state,
                  pkt->connStatus);
          goto action;
        }
        pcn_log(
            ctx, LOG_DEBUG,
            "[_HOOK] [ConntrackLabel] [ICMP] !FWD !REV should not get here ");
      } else {
        // miss, no entry in table
        if ((pkt->direction == DIRECTION_FORWARD) &&
            (icmp->type == ICMP_ECHO)) {
          pkt->connStatus = NEW;
          pkt->state = NEW;
          pkt->ttl = *timestamp + ICMP_TIMEOUT;
          pkt->sequence = icmp->un.echo.sequence;
          pkt->setMask = BIT(BIT_CONNTRACK);
          BIT_SET(pkt->mask, BIT(BIT_CT_SET_TTL) | BIT(BIT_CT_SET_SEQUENCE) |
                                    BIT(BIT_CT_SET_STATE) |
                                    BIT(BIT_CT_SET_MASK));
          pcn_log(ctx, LOG_DEBUG,
                  "[_HOOK] [ConntrackLabel] [ICMP] [MISS]  state [%d] -> [%d] "
                  "label [%d] ",
                  session_value_p->state, pkt->state, pkt->connStatus);
          goto action;
        }
      }
    } else {
      pcn_log(ctx, LOG_DEBUG,
              "[_HOOK] [ConntrackLabel] [ICMP] ![ECHO/ECHOREPLY] ");
    }

    pkt->connStatus = INVALID;
    goto action;
  }

  pcn_log(ctx, LOG_DEBUG,
          "[_HOOK] [ConntrackLabel] [l4proto = %d] not supported ",
          pkt->l4proto);

  // If it gets here, the protocol is not yet supported.
  pkt->connStatus = INVALID;
  goto action;

action:;

  pcn_log(ctx, LOG_DEBUG,
          "[_HOOK] [ConntrackLabel] [action] pkt->mask: 0x%x "
          "pkt->connStatus: %d ",
          pkt->mask, pkt->connStatus);

  // TODO possible optimization, inject it if needed
  int *decision = getForwardingDecision();

  if (decision == NULL) {
    return RX_DROP;
  }

#if _INGRESS_LOGIC

  switch (*decision) {
  case INPUT_LABELING:
    pcn_log(ctx, LOG_DEBUG, "[_HOOK] [ConntrackLabel] INPUT_LABELING ");

#if _CONNTRACK_MODE_INPUT == 2
    goto DISABLED;
#elif _CONNTRACK_MODE_INPUT == 1
    if (pkt->connStatus == ESTABLISHED) {
      incrementAcceptEstablishedInput(md->packet_len);
      goto ENABLED_MATCH;
    } else {
      goto ENABLED_NOT_MATCH;
    }
#endif
    return RX_DROP;

  case FORWARD_LABELING:
    pcn_log(ctx, LOG_DEBUG, "[_HOOK] [ConntrackLabel] FORWARD_LABELING ");

#if _CONNTRACK_MODE_FORWARD == 2
    goto DISABLED;
#elif _CONNTRACK_MODE_FORWARD == 1
    if (pkt->connStatus == ESTABLISHED) {
      incrementAcceptEstablishedForward(md->packet_len);
      goto ENABLED_MATCH;
    } else {
      goto ENABLED_NOT_MATCH;
    }
#endif
    return RX_DROP;

  case PASS_LABELING:
    pcn_log(ctx, LOG_DEBUG, "[_HOOK] [ConntrackLabel] PASS_LABELING ");

    goto DISABLED;
    return RX_DROP;

  default:
    pcn_log(ctx, LOG_ERR, "[_HOOK] [ConntrackLabel] Something went wrong. ");
    return RX_DROP;
  }

#endif

#if _EGRESS_LOGIC

  switch (*decision) {
  case OUTPUT_LABELING:
    pcn_log(ctx, LOG_DEBUG, "[_HOOK] [ConntrackLabel] OUTPUT_LABELING ");

#if _CONNTRACK_MODE_OUTPUT == 2
    goto DISABLED;
#elif _CONNTRACK_MODE_OUTPUT == 1
    if (pkt->connStatus == ESTABLISHED) {
      incrementAcceptEstablishedOutput(md->packet_len);
      goto ENABLED_MATCH;
    } else {
      goto ENABLED_NOT_MATCH;
    }
#endif
    return RX_DROP;

  case PASS_LABELING:
    pcn_log(ctx, LOG_DEBUG, "[_HOOK] [ConntrackLabel] PASS_LABELING ");

    goto DISABLED;
    return RX_DROP;

  default:
    pcn_log(ctx, LOG_ERR, "[_HOOK] [ConntrackLabel] Something went wrong. ");
    return RX_DROP;
  }

#endif

DISABLED:;
  pcn_log(
      ctx, LOG_TRACE,
      "[_HOOK] [ConntrackLabel] (Optimization OFF) Calling Chainforwarder %d",
      _CHAINFORWARDER);
  call_bpf_program(ctx, _CHAINFORWARDER);

  pcn_log(ctx, LOG_TRACE,
          "[_HOOK] [ConntrackLabel] Calling Chainforwarder FAILED. Dropping");
  return RX_DROP;

ENABLED_MATCH:;
  // ON (Perform optimization for accept established)
  //  if established, forward directly
  pcn_log(ctx, LOG_TRACE,
          "[_HOOK] [ConntrackLabel] (Optimization ON) ESTABLISHED Connection "
          "found. "
          "Calling ConntrackTableUpdate.");
  call_bpf_program(ctx, _CONNTRACKTABLEUPDATE);

  pcn_log(ctx, LOG_DEBUG, "[_HOOK] [ConntrackLabel] Something went wrong.");
  return RX_DROP;

ENABLED_NOT_MATCH:;
  // ON (Perform optimization for accept established), but no match on
  // ESTABLISHED connection
  pcn_log(
      ctx, LOG_TRACE,
      "[_HOOK] [ConntrackLabel] (Optimization ON) no connection found. Calling "
      "ChainForwarder.");
  call_bpf_program(ctx, _CHAINFORWARDER);

  pcn_log(ctx, LOG_DEBUG, "[_HOOK] [ConntrackLabel] Something went wrong.");

  return RX_DROP;
}
