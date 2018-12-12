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
   Match on IP Source
   ======================= */

#define TRACE _TRACE
//#include <bcc/helpers.h>

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

#if _NR_ELEMENTS > 0
struct elements {
  uint64_t bits[_MAXRULES];
};

struct lpm_k {
  u32 netmask_len;
  __be32 ip;
};

BPF_TABLE("extern", int, struct elements, sharedEle_DIRECTION, 1);
static __always_inline struct elements *getShared() {
  int key = 0;
  return sharedEle_DIRECTION.lookup(&key);
}

BPF_F_TABLE("lpm_trie", struct lpm_k, struct elements, ip_TYPETrie_DIRECTION,
            1024, BPF_F_NO_PREALLOC);

static __always_inline struct elements *getBitVect(struct lpm_k *key) {
  return ip_TYPETrie_DIRECTION.lookup(key);
}

#endif


BPF_TABLE("extern", int, struct packetHeaders, packet_DIRECTION, 1);
static __always_inline struct packetHeaders *getPacket() {
  int key = 0;
  return packet_DIRECTION.lookup(&key);
}

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
pcn_log(ctx, LOG_DEBUG, "Code Ip_TYPE_DIRECTION receiving packet. ");

/*The struct elements and the lookup table are defined only if NR_ELEMENTS>0, so
 * this code has to be used only in those cases.*/
#if _NR_ELEMENTS > 0
  int key = 0;
  struct packetHeaders *pkt = getPacket();
  if (pkt == NULL) {
    // Not possible
    return RX_DROP;
  }

  struct lpm_k lpm_key = {32, pkt->_TYPEIp};
  struct elements *ele = getBitVect(&lpm_key);
  if (ele == NULL) {
    pcn_log(ctx, LOG_DEBUG, "No match. (pkt->_TYPEIp: %u) ", pkt->_TYPEIp);
    _DEFAULTACTION
  } else {
    struct elements *result = getShared();
    if (result == NULL) {
      /*Can't happen. The PERCPU is preallocated.*/
      return RX_DROP;
    } else {
/*#pragma unroll does not accept a loop with a single iteration, so we need to
 * distinguish cases to avoid a verifier error.*/
#if _NR_ELEMENTS == 1
      (result->bits)[0] = (result->bits)[0] & (ele->bits)[0];
#else
#pragma unroll
      for (int i = 0; i < _NR_ELEMENTS; ++i) {
        /*This is the first module, it initializes the percpu*/
        (result->bits)[i] = (result->bits)[i] & (ele->bits)[i];
      }

#endif
    }  // if result == NULL
  }    // if ele==NULL
  call_ingress_program(ctx, _NEXT_HOP_1);

#else
  return RX_DROP;
#endif

  return RX_DROP;
}
