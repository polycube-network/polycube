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
  WILDCARD,
  NEW,
  ESTABLISHED,
  RELATED,
  INVALID,
  SYN_SENT,
  SYN_RECV,
  FIN_WAIT,
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
};

struct ct_k {
  uint32_t srcIp;
  uint32_t dstIp;
  uint8_t l4proto;
  uint16_t srcPort;
  uint16_t dstPort;
};

struct ct_v {
  uint64_t ttl;
  uint8_t state;
  uint32_t sequence;
};


#if _CONNTRACK_MODE != 0
BPF_TABLE("extern", struct ct_k, struct ct_v, connections, 10240);
#endif

BPF_TABLE("extern", int, struct packetHeaders, packet, 1);

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME][ConntrackTableUpdate]: Receiving packet");
#if _CONNTRACK_MODE == 0
  return RX_OK;
#else
  int k = 0;
  struct packetHeaders *pkt = packet.lookup(&k);
  if (pkt == NULL) {
    // Not possible
    return RX_DROP;
  }

  if (pkt->connStatus == INVALID) {
    // No business here for these packets
    goto forward_action;
  }

  struct ct_k key = {0, 0, 0, 0, 0};
  key.srcIp = pkt->srcIp;
  key.dstIp = pkt->dstIp;
  key.l4proto = pkt->l4proto;
  key.srcPort = pkt->srcPort;
  key.dstPort = pkt->dstPort;

  struct ct_k rev_key = {0, 0, 0, 0, 0};
  rev_key.srcIp = pkt->dstIp;
  rev_key.dstIp = pkt->srcIp;
  rev_key.l4proto = pkt->l4proto;
  rev_key.srcPort = pkt->dstPort;
  rev_key.dstPort = pkt->srcPort;

  struct ct_v newEntry = {0, 0, 0};
  struct ct_v *value;

  /* == UDP == */
  if (pkt->l4proto == IPPROTO_UDP) {
    value = connections.lookup(&key);
    if (value != NULL) {
      // Found in forward direction
      if (value->ttl <= bpf_ktime_get_ns()) {
        // Entry expired, so now it is to be treated as NEW.
        value->ttl = bpf_ktime_get_ns() + UDP_NEW_TIMEOUT;
        value->state = NEW;
        goto forward_action;
      } else {
        // Valid entry
        if (value->state == NEW) {
          // An entry was already present with the NEW state. This means that
          // there has been no answer, from the other side. Connection is still
          // NEW.
          // TODO: For now I am refreshing the TTL, this can lead to an DoS
          // attack where the attacker prevents the entry from being deleted by
          // continuosly sending packets.
          value->ttl = bpf_ktime_get_ns() + UDP_NEW_TIMEOUT;
          goto forward_action;
        } else {
          // value->state == ESTABLISHED
          value->ttl = bpf_ktime_get_ns() + UDP_ESTABLISHED_TIMEOUT;
          goto forward_action;
        }
      }
    }
    // If it gets here, the entry in the forward direction was not present

    // Checking if the entry is present in the reverse direction
    value = connections.lookup(&rev_key);
    if (value != NULL) {
      // Found in the reverse direction.
      if (value->ttl <= bpf_ktime_get_ns()) {
        // Entry expired, so now it is to be treated as NEW.
        value->ttl = bpf_ktime_get_ns() + UDP_NEW_TIMEOUT;
        value->state = NEW;
        goto forward_action;
      } else {
        if (value->state == NEW) {
          // An entry was present in the rev direction with the NEW state. This
          // means that this is an answer, from the other side. Connection is
          // now ESTABLISHED.
          value->ttl = bpf_ktime_get_ns() + UDP_NEW_TIMEOUT;
          value->state = ESTABLISHED;
          goto forward_action;
        } else {
          // value->state == ESTABLISHED
          value->ttl = bpf_ktime_get_ns() + UDP_ESTABLISHED_TIMEOUT;
          goto forward_action;
        }
      }
    }
    // No entry found in both directions. Create one.
    newEntry.ttl = bpf_ktime_get_ns() + UDP_NEW_TIMEOUT;
    newEntry.state = NEW;
    newEntry.sequence = 0;
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
      newEntry.ttl = bpf_ktime_get_ns() + ICMP_TIMEOUT;
      newEntry.state = NEW;
      newEntry.sequence = 0;
      connections.insert(&key, &newEntry);
      goto forward_action;
    }
    if (icmp->type == ICMP_ECHOREPLY) {
      // No more packets expected here.
      connections.delete(&rev_key);
      goto forward_action;
    }
    // All other ICMPs are not supported or RELATED (so nothing to do)
    // TODO: ICMP Related packet for now are not refreshing TTL. Is that right?
    goto forward_action;
  }

  /* == TCP  == */
  if (pkt->l4proto == IPPROTO_TCP) {
    // If it is a RST, label it as established.
    if ((pkt->flags & TCPHDR_RST) != 0) {
      connections.delete(&key);
      connections.delete(&rev_key);
      goto forward_action;
    }

    value = connections.lookup(&key);
    if (value != NULL) {
      // Found in forward direction
      if (value->state == SYN_SENT) {
        // Still haven't received a SYN,ACK To the SYN
        if ((pkt->flags & TCPHDR_SYN) != 0 &&
            (pkt->flags | TCPHDR_SYN) == TCPHDR_SYN) {
          // Another SYN. It is valid, probably a retransmission.
          value->ttl = bpf_ktime_get_ns() + TCP_SYN_SENT;
          goto forward_action;
        } else {
          // Receiving packets outside the 3-Way handshake without completing
          // the handshake
          // TODO: Drop it?
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
          value->ttl = bpf_ktime_get_ns() + TCP_ESTABLISHED;
          goto forward_action;
        } else {
          // Validation failed, either ACK is not the only flag set or the ack
          // number is wrong
          // TODO: drop it?
          pkt->connStatus = INVALID;
          goto forward_action;
        }
      }

      if (value->state == FIN_WAIT) {
        // Already received a FIN in rev direction, waiting the FIN from the
        // other side
        if ((pkt->flags & TCPHDR_FIN) != 0) {
          // FIN received. Let's wait for it to be acknowledged.
          value->state = LAST_ACK;
          value->ttl = bpf_ktime_get_ns() + TCP_LAST_ACK;
          value->sequence = pkt->seqN + HEX_BE_ONE;
          goto forward_action;
        } else {
          // Still receiving packets
          value->ttl = bpf_ktime_get_ns() + TCP_FIN_WAIT;
          goto forward_action;
        }
      }

      if (value->state == ESTABLISHED) {
        if ((pkt->flags & TCPHDR_FIN) != 0) {
          // Initiating closing sequence. Reversing the entry, such that the
          // next FIN will be found in forward way.
          connections.delete(&key);
          struct ct_v newEntry = {0, 0, 0};
          newEntry.state = FIN_WAIT;
          newEntry.ttl = bpf_ktime_get_ns() + TCP_FIN_WAIT;
          connections.update(&rev_key, &newEntry);
          goto forward_action;
        } else {
          value->ttl = bpf_ktime_get_ns() + TCP_ESTABLISHED;
          goto forward_action;
        }
      }

      if (value->state == TIME_WAIT) {
        // Let the packet go, but do not update timers.
        goto forward_action;
      }

      pcn_log(ctx, LOG_DEBUG,
              "[_CHAIN_NAME][ConntrackTableUpdate]: Should not get here. Flags: %d. "
              "State: %d. ",
              pkt->flags, value->state);
      // TODO Unexpected situation
      goto forward_action;
    }

    // If it gets here, the entry in the forward direction was not present
    value = connections.lookup(&rev_key);
    if (value != NULL) {
      // Found in reverse direction
      if (value->state == SYN_SENT) {
        // This should be a SYN, ACK answer
        if ((pkt->flags & TCPHDR_ACK) != 0 && (pkt->flags & TCPHDR_SYN) != 0 &&
            (pkt->flags | (TCPHDR_SYN | TCPHDR_ACK)) ==
                (TCPHDR_SYN | TCPHDR_ACK) &&
            pkt->ackN == value->sequence) {
          value->state = SYN_RECV;
          value->ttl = bpf_ktime_get_ns() + TCP_SYN_RECV;
          value->sequence = pkt->seqN + HEX_BE_ONE;
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
          value->ttl = bpf_ktime_get_ns() + TCP_SYN_RECV;
          goto forward_action;
        }
        pkt->connStatus = INVALID;
        goto forward_action;
      }

      if (value->state == ESTABLISHED) {
        if ((pkt->flags & TCPHDR_FIN) != 0) {
          // Initiating closing sequence
          value->state = FIN_WAIT;
          value->ttl = bpf_ktime_get_ns() + TCP_FIN_WAIT;
        } else {
          value->ttl = bpf_ktime_get_ns() + TCP_ESTABLISHED;
          goto forward_action;
        }
      }

      if (value->state == LAST_ACK) {
        if ((pkt->flags & TCPHDR_ACK && pkt->ackN == value->sequence) != 0) {
          // Ack to the last FIN.
          value->state = TIME_WAIT;
          value->ttl = bpf_ktime_get_ns() + TCP_LAST_ACK;
          goto forward_action;
        }
        // Still receiving packets
        value->ttl = bpf_ktime_get_ns() + TCP_LAST_ACK;
        goto forward_action;
      }

      pcn_log(ctx, LOG_DEBUG,
              "[_CHAIN_NAME][ConntrackTableUpdate]: Should not get here. Flags: %d. "
              "State: %d. ",
              pkt->flags, value->state);
      goto forward_action;
    }

    // New entry. It has to be a SYN.
    if ((pkt->flags & TCPHDR_SYN) != 0 &&
        (pkt->flags | TCPHDR_SYN) == TCPHDR_SYN) {
      newEntry.state = SYN_SENT;
      newEntry.ttl = bpf_ktime_get_ns() + TCP_SYN_SENT;
      newEntry.sequence = pkt->seqN + HEX_BE_ONE;
      connections.update(&key, &newEntry);
      goto forward_action;
    } else {
      // Validation failed
      pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME][ConntrackTableUpdate]: Validation failed %d", pkt->flags);
      goto forward_action;
    }
  }

forward_action:;
  return RX_OK;
#endif
}
