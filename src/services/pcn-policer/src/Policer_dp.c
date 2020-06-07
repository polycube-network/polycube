/*
 * Copyright 2020 The Polycube Authors
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


enum {
  ACTION_PASS,
  ACTION_LIMIT,
  ACTION_DROP
};

struct bucket {
  u64 tokens;       // 1 bit = 1000000 tokens
  u64 refill_rate;  // tokens/us
  u64 capacity;
  u64 last_update;  // timestamp in us
};

struct contract {
  u8 action;
  struct bucket bucket;
  struct bpf_spin_lock lock;
};

#define MAX_CONTRACTS 100000

#if POLYCUBE_PROGRAM_TYPE == 1  // EGRESS
BPF_TABLE("extern", int, struct contract, default_contract, 1);
BPF_TABLE("extern", u32, struct contract, contracts, MAX_CONTRACTS);
#else  // INGRESS
BPF_TABLE_SHARED("array", int, struct contract, default_contract, 1);
BPF_TABLE_SHARED("hash", u32, struct contract, contracts, MAX_CONTRACTS);
#endif


static inline int limit_rate(struct CTXTYPE *ctx, struct contract *contract) {
  struct bucket *bucket = &contract->bucket;
  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;

  u64 curtime = bpf_ktime_get_ns() / 1000; // In us
  
  bpf_spin_lock(&contract->lock);

  // Refill tokens
  // If last_update == 0 the bucket is new, no need to add tokens
  if (bucket->last_update == 0) {
    bucket->last_update = curtime;
  
  } else if (curtime > bucket->last_update){
    u64 new_tokens =
        (curtime - bucket->last_update) * bucket->refill_rate;
    if (new_tokens > 0) {
      bucket->tokens += new_tokens;
      if (bucket->tokens > bucket->capacity) {
        bucket->tokens = bucket->capacity;
      }
      bucket->last_update = curtime;
    }
  }

  // Consume tokens
  u32 needed_tokens = (data_end - data) * 8 * 1000000;
  u8 retval;
  if (bucket->tokens >= needed_tokens) {
    bucket->tokens -= needed_tokens;
    retval = RX_OK;
  } else {
    retval = RX_DROP;
  }

  bpf_spin_unlock(&contract->lock);

  return retval;
}

static __always_inline
int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  int zero = 0;

  // Retrieve contract
  struct contract *contract = contracts.lookup(&md->traffic_class);
  if (!contract) {
    contract = default_contract.lookup(&zero);
    if (!contract) {
      pcn_log(ctx, LOG_ERR, "Cannot access default contract");
      return RX_DROP;
    }
  }

  // Apply action
  switch (contract->action) {
    case ACTION_PASS:
      return RX_OK;
      break;

    case ACTION_LIMIT:
      return limit_rate(ctx, contract);
      break;

    case ACTION_DROP:
      return RX_DROP;
      break;
  }

  pcn_log(ctx, LOG_ERR, "Unknown action");
  return RX_DROP;
}