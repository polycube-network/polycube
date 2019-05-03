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

#if _NR_ELEMENTS > 0
struct elements {
  uint64_t bits[_MAXRULES];
};

BPF_ARRAY(actions, int, 10000);
static __always_inline int *getAction(int *key) {
  return actions.lookup(key);
}

BPF_TABLE("extern", int, struct elements, sharedEle, 1);
static __always_inline struct elements *getShared() {
  int key = 0;
  return sharedEle.lookup(&key);
}
#endif

BPF_TABLE("percpu_array", int, u64, pktsCounter, 8000);
BPF_TABLE("percpu_array", int, u64, bytesCounter, 8000);

static __always_inline void incrementCounters(int *action, u32 bytes) {
  u64 *value;
  value = pktsCounter.lookup(action);
  if (value) {
    *value += 1;
  }
  value = bytesCounter.lookup(action);
  if (value) {
    *value += bytes;
  }
}

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME][Action]: receiving packet");

#if _NR_ELEMENTS > 0
  int key = 0;
  struct elements *ruleMatched = getShared();
  if (ruleMatched == NULL) {
    /*Not possible*/
    return RX_DROP;
  }

  pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME][Action]: Rule matched: %d", (int)(ruleMatched->bits)[0]);

  key = (int)(ruleMatched->bits)[0];
  int *action = getAction(&key);
  if (action == NULL) {
    /*Not possible*/
    return RX_DROP;
  }

  incrementCounters(&key, md->packet_len);
  switch (*action) {
  case 0:
    pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME][Action]: Action taken: DROP");
    return RX_DROP;

  case 1:
    pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME][Action]: Action taken: FORWARD");
    call_next_program(ctx, _CONNTRACKTABLEUPDATE);
    return RX_DROP;

  default:
    pcn_log(ctx, LOG_ERR, "[_CHAIN_NAME][Action]: Something went wrong");
    return RX_DROP;
  }

#endif
  return RX_DROP;
}
