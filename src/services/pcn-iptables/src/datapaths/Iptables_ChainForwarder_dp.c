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
     Chain Forarder
   ======================= */

enum {
  INPUT_LABELING,   // goto input chain and label packet
  FORWARD_LABELING, // goto forward chain and label packet
  OUTPUT_LABELING,  // goto output chain and label packet
  PASS_LABELING,    // one chain is hit (IN/PUT/FWD) but there are no rules and default action is accept. Label packet and let it pass.
  PASS_NO_LABELING, // OUTPUT chain is not hit, let the packet pass without labeling //NEVER HIT
  DROP_NO_LABELING  // one chain is hit (IN/PUT/FWD) but there are no rules and default action is DROP. //NEVER HIT
};

// This is the percpu array containing the forwarding decision. ChainForwarder just lookup this value.
BPF_TABLE("extern", int, int, forwardingDecision, 1);

static __always_inline int *getForwardingDecision() {
  int key = 0;
  return forwardingDecision.lookup(&key);
}

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  pcn_log(ctx, LOG_DEBUG, "Code ChainForwarder receiving packet.");

  int * decision = getForwardingDecision();

  if (decision == NULL) {
    return RX_DROP;
  }

#if _INGRESS_LOGIC

  switch (*decision) {
  case INPUT_LABELING:
    pcn_log(ctx, LOG_DEBUG, "ChainForwarder: Call INPUT Chain %d ", _NEXT_HOP_INPUT_1);
    call_bpf_program(ctx, _NEXT_HOP_INPUT_1);
    return RX_DROP;

  case FORWARD_LABELING:
    pcn_log(ctx, LOG_DEBUG, "ChainForwarder: Call FORWARD Chain %d ", _NEXT_HOP_FORWARD_1);
    call_bpf_program(ctx, _NEXT_HOP_FORWARD_1);
    return RX_DROP;

  case PASS_LABELING:
    pcn_log(ctx, LOG_DEBUG, "ChainForwarder INGRESS: Call ConntrackTablesUpdate %d ", _CONNTRACKTABLEUPDATE);
    call_bpf_program(ctx, _CONNTRACKTABLEUPDATE);
    return RX_DROP;

  default:
    pcn_log(ctx, LOG_ERR, "ChainForwarder Ingress: Something went wrong. ");
    return RX_DROP;

  }

#endif

#if _EGRESS_LOGIC

  switch (*decision) {
  case OUTPUT_LABELING:
    pcn_log(ctx, LOG_DEBUG, "ChainForwarder: Call OUTPUT Chain %d ", _NEXT_HOP_OUTPUT_1);
    call_bpf_program(ctx, _NEXT_HOP_OUTPUT_1);
    return RX_DROP;

  case PASS_LABELING:
    pcn_log(ctx, LOG_DEBUG, "ChainForwarder EGRESS: Call ConntrackTablesUpdate %d ", _CONNTRACKTABLEUPDATE);
    call_bpf_program(ctx, _CONNTRACKTABLEUPDATE);
    return RX_DROP;

  default:
    pcn_log(ctx, LOG_ERR, "ChainForwarder Egress: Something went wrong. ");
    return RX_DROP;

  }

#endif

  return RX_DROP;
}