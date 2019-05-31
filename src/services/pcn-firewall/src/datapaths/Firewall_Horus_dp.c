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

/* =======================
   HORUS Mitigator
   ======================= */
// #include <uapi/linux/ip.h>
// #include <uapi/linux/udp.h>

enum {
  FORWARDING_NOT_SET,
  FORWARDING_PASS_LABELING
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
  uint8_t forwardingDecision;
} __attribute__((packed));

struct horusKey {
#if _SRCIP
  uint32_t srcIp;
#endif
#if _DSTIP
  uint32_t dstIp;
#endif
#if _SRCPORT
  uint16_t srcPort;
#endif
#if _DSTPORT
  uint16_t dstPort;
#endif
#if _L4PROTO
  uint8_t l4proto;
#endif
} __attribute__((packed));

struct horusValue {
  uint8_t action;
  uint32_t ruleID;
} __attribute__((packed));

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

BPF_TABLE("extern", int, struct packetHeaders, packet, 1);

// TODO 1024 -> const
BPF_TABLE("hash", struct horusKey, struct horusValue, horusTable, _MAX_RULE_SIZE_FOR_HORUS);

// Per-CPU maps used to keep counter of HORUS rules
BPF_TABLE("percpu_array", int, u64, pkts_horus, _MAX_RULE_SIZE_FOR_HORUS);
BPF_TABLE("percpu_array", int, u64, bytes_horus, _MAX_RULE_SIZE_FOR_HORUS);

static __always_inline void incrementHorusCounters(u32 ruleID, u32 bytes) {
  u64 *value;
  value = pkts_horus.lookup(&ruleID);
  if (value) {
    *value += 1;
  }
  value = bytes_horus.lookup(&ruleID);
  if (value) {
    *value += bytes;
  }
}

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME] [Horus] receiving packet.");

  // lookup pkt headers

  int zero = 0;

  struct packetHeaders *pkt;
  pkt = packet.lookup(&zero);

  if (pkt == NULL) {
    // Not possible
    return RX_DROP;
  }

  // build key
  struct horusKey key;

#if _SRCIP
  key.srcIp = pkt->srcIp;
#endif
#if _DSTIP
  key.dstIp = pkt->dstIp;
#endif
#if _SRCPORT
  key.srcPort = pkt->srcPort;
#endif
#if _DSTPORT
  key.dstPort = pkt->dstPort;
#endif
#if _L4PROTO
  key.l4proto = pkt->l4proto;
#endif

  // lookup key
  struct horusValue *value;
  value = horusTable.lookup(&key);

  pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME] [Horus] srcIp: %I dstIp: %I", pkt->srcIp, pkt->dstIp);

  if (value == NULL) {
    // Miss, goto pipleline
    goto PIPELINE;
  } else {
    // Independently from the final action (ACCEPT or DROP)
    // I have to update the counters
    if (value->ruleID < _MAX_RULE_SIZE_FOR_HORUS) {
      pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME] [Horus] RuleID: %d", value->ruleID);
      incrementHorusCounters(value->ruleID, md->packet_len);
    } else {
      pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME] [Horus] RuleID is greater than _MAX_RULE_SIZE_FOR_HORUS");
      goto PIPELINE;
    }

    if (value->action == 0) {
      pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME] [Horus] ACTION=DROP. Drop the packet. ");
      return RX_DROP;
    }
    if (value->action == 1) {
      // goto PASS
    #if _CONNTRACK_ENABLED
      pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME] [Horus] ACTION=ACCEPT & CT enabled. Tag with PASS_LABELING. ");
      pkt->forwardingDecision = FORWARDING_PASS_LABELING;
      call_next_program(ctx, _CONNTRACKLABEL);
      return RX_DROP;
    #else
      pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME] [Horus] ACTION=ACCEPT & CT disabled. Return RX_OK ");
      return RX_OK;
    #endif
    }
    goto PIPELINE;
  }

PIPELINE:;
  pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME] [Horus] Lookup MISS. Goto PIPELINE. ");
  pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME] [Horus] call CONNTRACKLABEL  _CONNTRACKLABEL ");
  call_next_program(ctx, _CONNTRACKLABEL);
  return RX_DROP;
}
