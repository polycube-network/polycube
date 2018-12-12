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

#include <bcc/helpers.h>

struct packetHeaders {
  uint64_t srcMac;
  uint64_t dstMac;
  uint16_t vlan;
  bool vlan_present;
  bool ip;
  uint32_t srcIp;
  uint32_t dstIp;
  uint8_t l4proto;
  uint16_t srcPort;
  uint16_t dstPort;
};

BPF_TABLE("extern", int, struct packetHeaders, packet, 1);

/* Bitmap managment */
#define BITS_PER_WORD (sizeof(uint32_t) * 8)
#define BIT_OFFSET(b) ((b) % BITS_PER_WORD)
#define GET_BIT(word, n) (word & (1ULL << BIT_OFFSET(n)))

struct rule {
  uint64_t srcMac;
  uint64_t dstMac;
  uint16_t vlanid;
  uint32_t srcIp;
  uint32_t dstIp;
  uint16_t lvl4_proto;
  uint16_t src_port;
  uint16_t dst_port;
  uint16_t inport;
  uint16_t action;
  uint16_t outport;
  uint32_t flags;  // Bitmap. The order is the same of the struct
};

struct action {
  uint16_t action;
  uint16_t outport;
};

BPF_HASH(rules, uint32_t, struct rule);
BPF_TABLE_SHARED("percpu_array", int, struct action, action, 1);

static __always_inline int handle_rx(struct CTXTYPE *ctx,
                                     struct pkt_metadata *md) {
  pcn_log(ctx, LOG_TRACE, "Matching module receiving packet");

  int j = 0;
  struct packetHeaders *pkt = 0;
  pkt = packet.lookup(&j);
  if (pkt == NULL)
    return RX_DROP;

  struct action *act = 0;
  int act_k = 0;
  act = action.lookup(&act_k);
  if (act == NULL)
    return RX_DROP;
  struct rule *r = 0;

#if RULES == 0
  pcn_log(ctx, LOG_DEBUG, "No rules. Dropping.");
  return RX_DROP;
#elif RULES == 1
  int key = 0;
  r = rules.lookup(&key);
  if (r == NULL) {
    // No rule with this ID
    goto NOT_MATCHED;
  }
#elif RULES > 1
#pragma unroll
  for (int i = 0; i < RULES; i++) {
    int key = i;
    r = rules.lookup(&key);
    if (r == NULL) {
      // No rule with this ID
      goto NOT_MATCHED;
    }
#endif
  /* Ethernet layer */
  if (GET_BIT(r->flags, 0))
    if (pkt->srcMac != r->srcMac)
      goto NOT_MATCHED;
  if (GET_BIT(r->flags, 1))
    if (pkt->dstMac != r->dstMac)
      goto NOT_MATCHED;
  if (GET_BIT(r->flags, 2))
    if (!(pkt->vlan_present && pkt->vlan == r->vlanid))
      goto NOT_MATCHED;
#if LEVEL > 2
  /* IP Layer */
  if (GET_BIT(r->flags, 3)) {
    if (pkt->ip != 1 || pkt->srcIp != r->srcIp)
      goto NOT_MATCHED;
  }
  if (GET_BIT(r->flags, 4)) {
    if (pkt->ip != 1 || pkt->dstIp != r->dstIp)
      goto NOT_MATCHED;
  }
#endif
#if LEVEL == 4
  /* Transport Layer */
  if (GET_BIT(r->flags, 5)) {
    if ((r->lvl4_proto == IPPROTO_TCP && pkt->l4proto != IPPROTO_TCP) ||
        (r->lvl4_proto == IPPROTO_UDP && pkt->l4proto != IPPROTO_UDP))
      goto NOT_MATCHED;
    if (GET_BIT(r->flags, 6))
      if (pkt->srcPort != r->src_port)
        goto NOT_MATCHED;
    if (GET_BIT(r->flags, 7))
      if (pkt->dstPort != r->dst_port)
        goto NOT_MATCHED;
  }
  /* Input port */
  if (GET_BIT(r->flags, 8))
    if (md->in_port != r->inport)
      goto NOT_MATCHED;
#endif
  /* If it gets here, the rule is matched. */
  act->action = r->action;
  act->outport = r->outport;
  call_ingress_program(ctx, 2);

NOT_MATCHED:;
#if RULES > 1
}  // Close for
#endif
pcn_log(ctx, LOG_DEBUG, "No rules. Dropping.");
return RX_DROP;
}
