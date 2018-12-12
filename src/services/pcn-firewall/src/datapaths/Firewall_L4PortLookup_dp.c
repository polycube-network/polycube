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
   Match on Transport Destination Port.
   ======================= */

#define TRACE _TRACE

//#include <bcc/helpers.h>
//#include <uapi/linux/in.h>
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17

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

BPF_TABLE("extern", int, struct packetHeaders, packet_DIRECTION, 1);
static __always_inline struct packetHeaders *getPacket() {
  int key = 0;
  return packet_DIRECTION.lookup(&key);
}

#if _NR_ELEMENTS > 0
struct elements {
  uint64_t bits[_MAXRULES];
};

BPF_TABLE("extern", int, struct elements, sharedEle_DIRECTION, 1);
static __always_inline struct elements *getShared() {
  int key = 0;
  return sharedEle_DIRECTION.lookup(&key);
}

BPF_HASH(_TYPEPorts_DIRECTION, uint16_t, struct elements);
static __always_inline struct elements *getBitVect(uint16_t *key) {
  return _TYPEPorts_DIRECTION.lookup(key);
}
#endif

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {

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


  uint16_t _TYPEPort = 0;
  if (pkt->l4proto != IPPROTO_TCP && pkt->l4proto != IPPROTO_UDP) {
    pcn_log(ctx, LOG_DEBUG, "Code _TYPEPort _DIRECTION ignoring packet. ");
    call_ingress_program(ctx, _NEXT_HOP_1);
    return RX_DROP;
  } else {
    pcn_log(ctx, LOG_DEBUG, "Code _TYPEPort _DIRECTION receiving packet. ");
    _TYPEPort = pkt->_TYPEPort;
  }

  struct elements *ele = getBitVect(&_TYPEPort);

  if (ele == NULL) {
    _TYPEPort = 0;
    ele = getBitVect(&_TYPEPort);
    if (ele == NULL) {
      pcn_log(ctx, LOG_DEBUG, "No match. ");
      _DEFAULTACTION
    }
  }
  struct elements *result = getShared();
  if (result == NULL) {
    /*Can't happen. The PERCPU is preallocated.*/
    return RX_DROP;
  } else {
/*#pragma unroll does not accept a loop with a single iteration, so we need to
 * distinguish cases to avoid a verifier error.*/
#if _NR_ELEMENTS == 1
    (result->bits)[0] = (ele->bits)[0] & (result->bits)[0];
#else
    int i = 0;
#pragma unroll
    for (i = 0; i < _NR_ELEMENTS; ++i) {
      (result->bits)[i] = (result->bits)[i] & (ele->bits)[i];
    }

#endif
  }  // if result == NULL

  call_ingress_program(ctx, _NEXT_HOP_1);
#else
  return RX_DROP;
#endif

  return RX_DROP;
}
