/*
 * Copyright 2019 The Polycube Authors
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

/* =======================
   Action Cache Module
   ======================= */

#define IPPROTO_ICMP 1

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

enum {
  DIRECTION_FORWARD,  // Forward direction in session table
  DIRECTION_REVERSE   // Reverse direction in session table
};

struct tts_k {
  uint32_t srcIp;
  uint32_t dstIp;
  uint8_t l4proto;
  uint16_t srcPort;
  uint16_t dstPort;
} __attribute__((packed));

struct tts_v {
  uint32_t sessionId;
  uint8_t direction;
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

#define SESSION_DIM _SESSION_DIM
#define TUPLETOSESSION_DIM SESSION_DIM * 2

#define RANDOM_INDEX_RETRY 8

#if _INGRESS_LOGIC
BPF_TABLE_SHARED("lru_hash", struct tts_k, struct tts_v, tupletosession,
                 TUPLETOSESSION_DIM);
#endif

#if _EGRESS_LOGIC
BPF_TABLE("extern", struct tts_k, struct tts_v, tupletosession,
          TUPLETOSESSION_DIM);
#endif

#if _INGRESS_LOGIC
BPF_TABLE_SHARED("array", uint32_t, struct session_v, session, SESSION_DIM);
#endif

#if _EGRESS_LOGIC
BPF_TABLE("extern", uint32_t, struct session_v, session, SESSION_DIM);
#endif

BPF_TABLE("extern", int, struct packetHeaders, packet, 1);

static inline uint32_t get_free_index() {
  return (bpf_get_prandom_u32() % (SESSION_DIM));
}

// TODO use a userspace cleanup thread to free exipred entries
static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  pcn_log(ctx, LOG_DEBUG, "[_HOOK] [ActionCache] receiving packet.");

  int k = 0;
  struct packetHeaders *pkt = packet.lookup(&k);
  if (pkt == NULL) {
    return RX_DROP;
  }

  struct tts_k tuple;
  tuple.srcIp = pkt->srcIp;
  tuple.dstIp = pkt->dstIp;
  tuple.l4proto = pkt->l4proto;
  tuple.srcPort = pkt->srcPort;
  tuple.dstPort = pkt->dstPort;

  // check pkt against tupletosession table
  struct tts_v *tts_v_p = tupletosession.lookup(&tuple);
  if (tts_v_p == NULL) {
    // miss in tupletosession table -> allocate new index and push new entries

    // get new index
    uint32_t new_index = 0;

    struct session_v *session_v_p = NULL;

#pragma unroll
    for (int k = 0; k < RANDOM_INDEX_RETRY; k++) {
      new_index = get_free_index();
      session_v_p = session.lookup(&new_index);
      if (session_v_p == NULL) {
        goto drop;
      }
      if (session_v_p->setMask == 0) {
        goto new_index_found;
      }
    }

  drop:;
    pcn_log(ctx, LOG_DEBUG, "[_HOOK] [ActionCache] drop ");
    return RX_DROP;

  new_index_found:;
    pcn_log(ctx, LOG_DEBUG,
            "[_HOOK] [ActionCache] tuple src:%I dst:%I sport:%P ", tuple.srcIp,
            tuple.dstIp, tuple.srcPort);
    pcn_log(ctx, LOG_DEBUG,
            "[_HOOK] [ActionCache] tuple dport:%P - MISS - new_index:%d ",
            tuple.dstPort, new_index);

    // set session info into pkt metadata
    pkt->sessionId = new_index;
    pkt->direction = DIRECTION_FORWARD;

    pcn_log(ctx, LOG_DEBUG, "[_HOOK] [ActionCache] sessionId:%d direction:%d ",
            pkt->sessionId, pkt->direction);

    session_v_p->setMask = 0;
    session_v_p->actionMask = 0;

    // TODO when nat module will be added, this module will not be in charge of
    // pushing both fwd and rev keys

    // update forward key
    struct tts_v tts_v_new;
    tts_v_new.direction = DIRECTION_FORWARD;
    tts_v_new.sessionId = new_index;

    tupletosession.update(&tuple, &tts_v_new);

    // update rev key
    tts_v_new.direction = DIRECTION_REVERSE;

    tuple.srcIp = pkt->dstIp;
    tuple.dstIp = pkt->srcIp;
    if (pkt->l4proto != IPPROTO_ICMP) {
      tuple.srcPort = pkt->dstPort;
      tuple.dstPort = pkt->srcPort;
    }

    tupletosession.update(&tuple, &tts_v_new);

    // TODO right now, we always go to ctlabeling since we want to execute
    // *ONLY* conntrack module
    // in the future, with nat, we should always go to nat slowpath

    goto ctlabel;
  } else {
    // hit in tupletosession table

    // set session info into pkt metadata
    pkt->sessionId = tts_v_p->sessionId;
    pkt->direction = tts_v_p->direction;

    pcn_log(ctx, LOG_DEBUG,
            "[_HOOK] [ActionCache] tuple src:%I dst:%I sport:%P ", tuple.srcIp,
            tuple.dstIp, tuple.srcPort);
    pcn_log(ctx, LOG_DEBUG, "[_HOOK] [ActionCache] tuple dport:%P - HIT ",
            tuple.dstPort);

    pcn_log(ctx, LOG_DEBUG, "[_HOOK] [ActionCache] sessionId:%d direction:%d ",
            pkt->sessionId, pkt->direction);

    // TODO right now, we always go to ctlabeling since we want to execute
    // *ONLY* conntrack module
    // in the future, with nat, we should also check set fields before taking
    // slow/fast path

    goto ctlabel;
  }

ctlabel:;
#if _INGRESS_LOGIC
  call_bpf_program(ctx, _CONNTRACKLABEL_INGRESS);
#endif
#if _EGRESS_LOGIC
  call_bpf_program(ctx, _CONNTRACKLABEL_EGRESS);
#endif

  return RX_DROP;
}
