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

/* =======================
   Match on Transport Protocol.
   ======================= */

// _DIRECTION = {INPUT, FORWARD, OUTPUT}
// _TYPE = {src, dst}

// #include <bcc/helpers.h>
// #include <uapi/linux/in.h>

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

BPF_TABLE("extern", int, struct packetHeaders, packet, 1);
static __always_inline struct packetHeaders *getPacket() {
  int key = 0;
  return packet.lookup(&key);
}

#if _NR_ELEMENTS > 0
struct elements {
  uint64_t bits[_MAXRULES];
};

BPF_HASH(transportProto_DIRECTION, uint8_t, struct elements);
static __always_inline struct elements *getBitVect(uint8_t *key) {
  return transportProto_DIRECTION.lookup(key);
}

BPF_TABLE("extern", int, struct elements, sharedEle, 1);
static __always_inline struct elements *getShared() {
  int key = 0;
  return sharedEle.lookup(&key);
}
#endif

BPF_TABLE("extern", int, u64, pkts_default__DIRECTION, 1);
BPF_TABLE("extern", int, u64, bytes_default__DIRECTION, 1);
static __always_inline void incrementDefaultCounters_DIRECTION(u32 bytes) {
  u64 *value;
  int zero = 0;
  value = pkts_default__DIRECTION.lookup(&zero);
  if (value) {
    *value += 1;
  }

  value = bytes_default__DIRECTION.lookup(&zero);
  if (value) {
    *value += bytes;
  }
}

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  pcn_log(ctx, LOG_TRACE,
          "[_HOOK] [L4Protocol] [_DIRECTION] receiving packet. ");

/*The struct elements and the lookup table are defined only if _NR_ELEMENTS>0,
 * so
 * this code has to be used only in this case.*/
#if _NR_ELEMENTS > 0
  int key = 0;
  struct packetHeaders *pkt = getPacket();
  if (pkt == NULL) {
    // Not possible
    return RX_DROP;
  }

  uint8_t proto = pkt->l4proto;
  struct elements *ele = getBitVect(&proto);

  if (ele == NULL) {
    proto = 0;
    ele = getBitVect(&proto);
    if (ele == NULL) {
      pcn_log(
          ctx, LOG_TRACE,
          "[_HOOK] [L4Protocol] [_DIRECTION] no match. dropping proto 0x%x ",
          proto);
      incrementDefaultCounters_DIRECTION(md->packet_len);
      _DEFAULTACTION
    }
  }
  key = 0;
  struct elements *result = getShared();
  if (result == NULL) {
    /*Can't happen. The PERCPU is preallocated.*/
    return RX_DROP;
  } else {
    bool isAllZero = true;
/*#pragma unroll does not accept a loop with a single iteration, so we need to
 * distinguish cases to avoid a verifier error.*/
#if _NR_ELEMENTS == 1
    (result->bits)[0] = (ele->bits)[0] & (result->bits)[0];
    if (result->bits[0])
      isAllZero = false;
#else
    int i = 0;
#pragma unroll
    for (i = 0; i < _NR_ELEMENTS; ++i) {
      (result->bits)[i] = (result->bits)[i] & (ele->bits)[i];

      if (result->bits[i])
        isAllZero = false;
    }

#endif
    if (isAllZero) {
      pcn_log(ctx, LOG_TRACE,
              "[_HOOK] [L4Protocol] [_DIRECTION] bitvector is all zero. break "
              "pipeline ");
      incrementDefaultCounters_DIRECTION(md->packet_len);
      _DEFAULTACTION
    }
  }  // if result == NULL

  call_bpf_program(ctx, _NEXT_HOP_1);
#else
  return RX_DROP;
#endif

  return RX_DROP;
}
