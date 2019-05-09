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

BPF_TABLE("percpu_array", int, u64, pktsCounter, 1);
BPF_TABLE("percpu_array", int, u64, bytesCounter, 1);

static __always_inline void incrementCounters(u32 bytes) {
  u64 *value;
  int zero = 0;
  value = pktsCounter.lookup(&zero);
  if (value) {
    *value += 1;
  }
  value = bytesCounter.lookup(&zero);
  if (value) {
    *value += bytes;
  }
}

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME][DefaultAction]: Receiving packet");
  incrementCounters(md->packet_len);
  _ACTION

  return RX_DROP;
}
