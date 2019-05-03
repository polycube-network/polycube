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

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME][ChainForwarder]: Receiving packet");
#if defined(_INGRESS_LOGIC)
#if _NR_ELEMENTS_INGRESS > 0
    call_ingress_program(ctx, _NEXT_HOP_INGRESS_1);
#endif
    pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME][ChainForwarder]: No ingress chain");
    _DEFAULTACTION_INGRESS
#endif

#if defined(_EGRESS_LOGIC)
#if _NR_ELEMENTS_EGRESS > 0
    call_egress_program(ctx, _NEXT_HOP_EGRESS_1);
#endif
    pcn_log(ctx, LOG_DEBUG, "[_CHAIN_NAME][ChainForwarder]: No egress chain");
    _DEFAULTACTION_EGRESS
#endif
  pcn_log(ctx, LOG_ERR, "[_CHAIN_NAME][ChainForwarder]: miss");
  return RX_DROP;
}
