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
   Parse packet
   ======================= */

#define TRACE _TRACE

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  pcn_log(ctx, LOG_DEBUG, "Code ChainForwarder receiving packet.");
  if (md->in_port == _INGRESSPORT){
    #if _NR_ELEMENTS_INGRESS > 0
    call_ingress_program(ctx, _NEXT_HOP_INGRESS_1);
    #endif
    pcn_log(ctx, LOG_DEBUG, "No ingress chain. ");
    _DEFAULTACTION_INGRESS
  }

  if (md->in_port == _EGRESSPORT){
    #if _NR_ELEMENTS_EGRESS > 0
    call_ingress_program(ctx, _NEXT_HOP_EGRESS_1);
    #endif
    pcn_log(ctx, LOG_DEBUG, "No egress chain. ");
    _DEFAULTACTION_EGRESS
  }
  return RX_DROP;
}
