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

/* ==========================
   Match on Interface IN/OUT.
   ========================== */

// _DIRECTION = {INPUT, FORWARD, OUTPUT}
// _TYPE = {in, out}

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

// PERCPU ARRAY
// with parsed headers for current packet
BPF_TABLE("extern", int, struct packetHeaders, packet, 1);
static __always_inline struct packetHeaders *getPacket() {
  int key = 0;
  return packet.lookup(&key);
}

#if _NR_ELEMENTS > 0
struct elements {
  uint64_t bits[_MAXRULES];
};

BPF_TABLE("extern", int, struct elements, sharedEle, 1);
static __always_inline struct elements *getShared() {
  int key = 0;
  return sharedEle.lookup(&key);
}

BPF_HASH(_TYPEInterfaces_DIRECTION, uint16_t, struct elements);
static __always_inline struct elements *getBitVect(uint16_t *key) {
  return _TYPEInterfaces_DIRECTION.lookup(key);
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
#if _WILDCARD_RULE
  u64 wildcard_ele[_MAXRULES] = _WILDCARD_BITVECTOR;
#endif

  pcn_log(ctx, LOG_TRACE,
          "[_HOOK] [Interface] [_TYPE] [_DIRECTION] receiving packet. ");

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

  // TODO check if we should not use htons here.
  uint16_t _TYPEInterface = md->in_port;
  pcn_log(ctx, LOG_TRACE,
          "[_HOOK] [interface] [_TYPE] [_DIRECTION] current index: %d ",
          _TYPEInterface);

  // Interfaces are stored in an hashmap
  // A. map[0] (if exists) contains wildcard match
  // B. map[interface] (if some exists) contain interface matching

  // if no match in A. and B.
  // we assume current bitvector is 0x000000...
  // also AND returns 0x0000...
  // so we can apply DEFAULT action with no additional cost.

  struct elements *result = getShared();

  bool isAllZero = true;
  if (result == NULL) {
    /*Can't happen. The PERCPU is preallocated.*/
    return RX_DROP;
  } else {
    struct elements *ele = getBitVect(&_TYPEInterface);

    if (ele == NULL) {
// if lookup with interface fails, we have to
// a. verify if we have a wildcard key (0)
// b. if so, use to bitvector from wildcard key

#if _WILDCARD_RULE
      pcn_log(ctx, LOG_TRACE,
              "[_HOOK] [Interface] [_TYPE] [_DIRECTION] wildcard rule ");
      goto WILDCARD;
#else
      pcn_log(ctx, LOG_TRACE,
              "[_HOOK] [Interface] [_TYPE] [_DIRECTION] no match ");
      incrementDefaultCounters_DIRECTION(md->packet_len);
      _DEFAULTACTION
#endif
    }

/*#pragma unroll does not accept a loop with a single iteration, so we need to
* distinguish cases to avoid a verifier error.*/
#if _NR_ELEMENTS == 1
    (result->bits)[0] = (ele->bits)[0] & (result->bits)[0];
    if (result->bits[0] != 0)
      isAllZero = false;
    goto NEXT;

#if _WILDCARD_RULE
  WILDCARD:;
    (result->bits)[0] = wildcard_ele[0] & (result->bits)[0];
    if (result->bits[0] != 0)
      isAllZero = false;
#endif
#else
    int i = 0;
#pragma unroll
    for (i = 0; i < _NR_ELEMENTS; ++i) {
      (result->bits)[i] = (result->bits)[i] & (ele->bits)[i];
      if (result->bits[i] != 0)
        isAllZero = false;
    }
    goto NEXT;
#if _WILDCARD_RULE
  WILDCARD:;
#pragma unroll
    for (i = 0; i < _NR_ELEMENTS; ++i) {
      (result->bits)[i] = wildcard_ele[i] & (result->bits)[i];
      if (result->bits[i] != 0)
        isAllZero = false;
    }
#endif
#endif
  }  // if result == NULL

NEXT:;
  if (isAllZero) {
    pcn_log(ctx, LOG_TRACE,
            "[_HOOK] [Interface] [_TYPE] [_DIRECTION] bitvector is all zero. "
            "break pipeline ");
    incrementDefaultCounters_DIRECTION(md->packet_len);
    _DEFAULTACTION
  }
  call_bpf_program(ctx, _NEXT_HOP_1);
#else
  return RX_DROP;
#endif

  return RX_DROP;
}
