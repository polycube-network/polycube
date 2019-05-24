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

#define TCPHDR_FIN 0x01
#define TCPHDR_SYN 0x02
#define TCPHDR_RST 0x04
#define TCPHDR_ACK 0x10

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

#if defined(_INGRESS_LOGIC)
BPF_TABLE_SHARED("lru_hash", struct ct_k, struct ct_v, connections, 65536);
#elif defined(_EGRESS_LOGIC)
BPF_TABLE("extern", struct ct_k, struct ct_v, connections, 65536);
#else
#error "_INGRESS_LOGIC or _EGRESS_LOGIC should be defined"
#endif

BPF_TABLE("extern", int, struct packetHeaders, packet, 1);

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME][ConntrackLabel]: Receiving packet");
  int k = 0;
  struct packetHeaders *pkt = packet.lookup(&k);
  if (pkt == NULL) {
    // Not possible
    return RX_DROP;
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

  struct ct_v *value;
  struct ct_v newEntry = {0, 0, 0, 0, 0};

  /* == TCP  == */
  if (pkt->l4proto == IPPROTO_TCP) {
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

      // If it is a RST, label it as established.
      if ((pkt->flags & TCPHDR_RST) != 0) {
        pkt->connStatus = ESTABLISHED;
        goto action;
      }

      if (value->state == SYN_SENT) {
        // Still haven't received a SYN,ACK To the SYN
        if ((pkt->flags & TCPHDR_SYN) != 0 &&
            (pkt->flags | TCPHDR_SYN) == TCPHDR_SYN) {
          // Another SYN. It is valid, probably a retransmission.
          // connections.delete(&key);
          pkt->connStatus = NEW;
          goto action;
        } else {
          // Receiving packets outside the 3-Way handshake without completing
          // the handshake
          // TODO: Drop it?
          pkt->connStatus = INVALID;
          goto action;
        }
      }
      if (value->state == SYN_RECV) {
        // Expecting an ACK here
        if ((pkt->flags & TCPHDR_ACK) != 0 &&
            (pkt->flags | TCPHDR_ACK) == TCPHDR_ACK &&
            (pkt->ackN == value->sequence)) {
          // Valid ACK to the SYN, ACK
          pkt->connStatus = ESTABLISHED;
          goto action;
        } else {
          // Validation failed, either ACK is not the only flag set or the ack
          // number is wrong
          // TODO: drop it?
          pkt->connStatus = INVALID;
          goto action;
        }
      }

      if (value->state == ESTABLISHED || value->state == FIN_WAIT_1 ||
          value->state == FIN_WAIT_2 || value->state == LAST_ACK) {
        pkt->connStatus = ESTABLISHED;
        goto action;
      }

      if (value->state == TIME_WAIT) {
        // If the state is TIME_WAIT but we receive a new SYN the connection is
        // considered NEW
        if ((pkt->flags & TCPHDR_SYN) != 0 &&
            (pkt->flags | TCPHDR_SYN) == TCPHDR_SYN) {
          pkt->connStatus = NEW;
          goto action;
        }
      }

      // Unexpected situation
      pcn_log(ctx, LOG_DEBUG,
              "[FW_DIRECTION] Should not get here. Flags: %d. State: %d. ",
              pkt->flags, value->state);
      pkt->connStatus = INVALID;
      goto action;

    TCP_REVERSE:;

      // If it is a RST, label it as established.
      if ((pkt->flags & TCPHDR_RST) != 0) {
        pkt->connStatus = ESTABLISHED;
        goto action;
      }

      if (value->state == SYN_SENT) {
        // This should be a SYN, ACK answer
        if ((pkt->flags & TCPHDR_ACK) != 0 && (pkt->flags & TCPHDR_SYN) != 0 &&
            (pkt->flags | (TCPHDR_SYN | TCPHDR_ACK)) ==
                (TCPHDR_SYN | TCPHDR_ACK) &&
            pkt->ackN == value->sequence) {
          pkt->connStatus = ESTABLISHED;
          goto action;
        }
        // Here is an unexpected packet, only a SYN, ACK is acepted as an answer
        // to a SYN
        // TODO: Drop it?
        pkt->connStatus = INVALID;
        goto action;
      }

      if (value->state == SYN_RECV) {
        // The only acceptable packet in SYN_RECV here is a SYN,ACK
        // retransmission
        if ((pkt->flags & TCPHDR_ACK) != 0 && (pkt->flags & TCPHDR_SYN) != 0 &&
            (pkt->flags | (TCPHDR_SYN | TCPHDR_ACK)) ==
                (TCPHDR_SYN | TCPHDR_ACK) &&
            pkt->ackN == value->sequence) {
          pkt->connStatus = ESTABLISHED;
          goto action;
        }
        pkt->connStatus = INVALID;
        goto action;
      }

      if (value->state == ESTABLISHED || value->state == FIN_WAIT_1 ||
          value->state == FIN_WAIT_2 || value->state == LAST_ACK) {
        pkt->connStatus = ESTABLISHED;
        goto action;
      }

      if (value->state == TIME_WAIT) {
        // If the state is TIME_WAIT but we receive a new SYN the connection is
        // considered NEW
        if ((pkt->flags & TCPHDR_SYN) != 0 &&
            (pkt->flags | TCPHDR_SYN) == TCPHDR_SYN) {
          pkt->connStatus = NEW;
          goto action;
        }
      }

      pcn_log(ctx, LOG_DEBUG,
              "[ConntrackLabel] [REV_DIRECTION] Should not get here. Flags: "
              "%d. State: %d. ",
              pkt->flags, value->state);
      pkt->connStatus = INVALID;
      goto action;
    }

  TCP_MISS:;

    // New entry. It has to be a SYN.
    if ((pkt->flags & TCPHDR_SYN) != 0 &&
        (pkt->flags | TCPHDR_SYN) == TCPHDR_SYN) {
      pkt->connStatus = NEW;
      goto action;
    } else {
      // Validation failed
      // TODO: drop it?
      pkt->connStatus = INVALID;
      goto action;
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
        pkt->connStatus = NEW;
        goto action;
      } else {
        // value->state == ESTABLISHED
        pkt->connStatus = ESTABLISHED;
        goto action;
      }

    UDP_REVERSE:;

      if (value->state == NEW) {
        // An entry was present in the rev direction with the NEW state. This
        // means that this is an answer, from the other side. Connection is
        // now ESTABLISHED.
        pkt->connStatus = ESTABLISHED;
        goto action;
      } else {
        // value->state == ESTABLISHED
        pkt->connStatus = ESTABLISHED;
        goto action;
      }
    }

  UDP_MISS:;

    // No entry found in both directions. Create one.
    pkt->connStatus = NEW;
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
    if (icmp->type == ICMP_ECHO) {
      // Echo request is always treated as the first of the connection
      pkt->connStatus = NEW;
      goto action;
    }

    if (icmp->type == ICMP_ECHOREPLY) {
      value = connections.lookup(&key);
      if (value != NULL) {
        if ((value->ipRev != ipRev) && (value->portRev != portRev)) {
          goto ICMP_REVERSE;
        } else {
          goto ICMP_MISS;
        }

      ICMP_REVERSE:;

        pkt->connStatus = ESTABLISHED;
        goto action;
      } else {
        // A reply without a request
        // TODO drop it?
        pkt->connStatus = INVALID;
        goto action;
      }
    }

  ICMP_MISS:;

    if (icmp->type == ICMP_TIMESTAMP || icmp->type == ICMP_TIMESTAMPREPLY ||
        icmp->type == ICMP_INFO_REQUEST || icmp->type == ICMP_INFO_REPLY ||
        icmp->type == ICMP_ADDRESS || icmp->type == ICMP_ADDRESSREPLY) {
      // Not yet supported
      pkt->connStatus = INVALID;
      goto action;
    }

    // Here there are only ICMP errors
    // Error messages always include a copy of the offending IP header and up to
    // 8 bytes of the data that caused the host or gateway to send the error
    // message.
    if (data + 34 + sizeof(struct icmphdr) + sizeof(struct iphdr) > data_end) {
      return RX_DROP;
    }
    struct iphdr *encapsulatedIp = data + 34 + sizeof(struct icmphdr);
    if (encapsulatedIp->saddr <= encapsulatedIp->daddr) {
      key.srcIp = encapsulatedIp->saddr;
      key.dstIp = encapsulatedIp->daddr;
      ipRev = 0;
    } else {
      key.srcIp = encapsulatedIp->daddr;
      key.dstIp = encapsulatedIp->saddr;
      ipRev = 1;
    }

    key.l4proto = encapsulatedIp->protocol;

    if (data + 34 + sizeof(struct icmphdr) + sizeof(struct iphdr) + 8 >
        data_end) {
      return RX_DROP;
    }
    uint16_t *temp = data + 34 + sizeof(struct icmphdr) + sizeof(struct iphdr);
    key.srcPort = *temp;
    temp = data + 34 + sizeof(struct icmphdr) + sizeof(struct iphdr) + 2;
    key.dstPort = *temp;

    if (key.srcPort <= key.dstPort) {
      portRev = 0;
    } else {
      *temp = key.srcPort;
      key.srcPort = key.dstPort;
      key.dstPort = *temp;
      portRev = 1;
    }

    value = connections.lookup(&key);
    if (value != NULL) {
      pkt->connStatus = RELATED;
      goto action;
    }

    // If it gets here, this error is an answer to a packet not known or to an
    // expired connection.
    // TODO: drop it?
    pkt->connStatus = INVALID;
    goto action;
  }

  pcn_log(ctx, LOG_DEBUG, "Conntrack does not support the l4proto= %d",
          pkt->l4proto);

  // If it gets here, the protocol is not yet supported.
  pkt->connStatus = INVALID;
  goto action;

action:
#if _CONNTRACK_MODE == 1
  pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME][ConntrackLabel][Label=%d] Conntrack mode == 1 calling chainforwarder", pkt->connStatus);
  // Manual mode
  call_next_program(ctx, _CHAINFORWARDER);
  return RX_DROP;
#elif _CONNTRACK_MODE == 2
  // Automatic mode: if established, forward directly
  pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME][ConntrackLabel]: Conntrack mode == 2 ");

  if (pkt->connStatus == ESTABLISHED) {
    pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME][ConntrackLabel]: Conntrack mode == 2 ESTABLISHED calling ct table update");
    call_next_program(ctx, _CONNTRACKTABLEUPDATE);
  } else {
    pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME][ConntrackLabel]: Conntrack mode == 2 ESTABLISHED calling chain fwd");
    call_next_program(ctx, _CHAINFORWARDER);
  }
  pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME][ConntrackLabel]: Something went wrong.");
  return RX_DROP;
#endif
  return RX_DROP;
}
