
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


// Template parameters:
// DIRECTION direction (ingress/egress) of the program
// CLASSIFY  whether classification is enabled (0/1)
// NEXT      index of the active classification program (1/2)


#define _DIRECTION_PROGRAM
#ifdef ingress_PROGRAM
BPF_TABLE_SHARED("array", int, u16, index64, 64);
#endif


static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
#if _CLASSIFY
  pcn_log(ctx, LOG_TRACE, "_DIRECTION selector: redirecting to program _NEXT");

  call__DIRECTION_program(ctx, _NEXT);
  
  return RX_DROP;

#else
  pcn_log(ctx, LOG_TRACE, "_DIRECTION selector: classification disabled");
  return RX_OK;
#endif
}