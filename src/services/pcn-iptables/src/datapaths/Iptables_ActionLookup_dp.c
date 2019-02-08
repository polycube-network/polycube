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
   Action on matched rule
   ======================= */

// ALL _SOMETHING are const changed dynamically by the control plane

#if _NR_ELEMENTS > 0
struct elements {
  uint64_t bits[_MAXRULES];
};

// Table mapping rule -> actions
// rule id
// action int () Look at switch case
// TODO Use enum

// _DIRECTION -> ingress/egress injected by control plane
// TODO define 10000 as a constant

BPF_ARRAY(actions_DIRECTION, int, 10000);

// Hide actions_DIRECTION name for the following datapath
static __always_inline int *getAction(int *key) {
  return actions_DIRECTION.lookup(key);
}

// sharedEle vector of uint64
// current bitvector matched
// percpu_array of 1 element
BPF_TABLE("extern", int, struct elements, sharedEle, 1);
static __always_inline struct elements *getShared() {
  int key = 0;
  return sharedEle.lookup(&key);
}
#endif

// Counters
// TODO define 8000 as a const
BPF_TABLE("percpu_array", int, u64, pkts_DIRECTION, 8000);
BPF_TABLE("percpu_array", int, u64, bytes_DIRECTION, 8000);

static __always_inline void incrementCounters(int *action, u32 bytes) {
  u64 *value;
  value = pkts_DIRECTION.lookup(action);
  if (value) {
    *value += 1;
  }
  value = bytes_DIRECTION.lookup(action);
  if (value) {
    *value += bytes;
  }
}

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  pcn_log(ctx, LOG_DEBUG, "Code action _DIRECTION receiving packet.");

// _NR_ELEMENTS = int (number of rules / 64)
// because of a bug in bcc, 63 bits are used
#if _NR_ELEMENTS > 0
  int key = 0;
  struct elements *ruleMatched = getShared();
  if (ruleMatched == NULL) {
    /*Not possible*/
    return RX_DROP;
  }

  pcn_log(ctx, LOG_DEBUG, "Rule matched: %d ", (int)(ruleMatched->bits)[0]);

  // first element of the bitvector is the matched rule!
  key = (int)(ruleMatched->bits)[0];
  int *action = getAction(&key);
  if (action == NULL) {
    /*Not possible*/
    return RX_DROP;
  }

  incrementCounters(&key, md->packet_len);
  switch (*action) {
  case 0:
    pcn_log(ctx, LOG_DEBUG, "Action taken: DROP ");
    return RX_DROP;

  case 1:
    pcn_log(ctx, LOG_DEBUG,
            "Action taken: ACCEPT (call CONNTRACKTABLEUPDATE) ");
    call_bpf_program(ctx, _CONNTRACKTABLEUPDATE);
    return RX_DROP;

  default:
    pcn_log(ctx, LOG_ERR, "Something went wrong. ");
    return RX_DROP;
  }

#endif
  return RX_DROP;
}
