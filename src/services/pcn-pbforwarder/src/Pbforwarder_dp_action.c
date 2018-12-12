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

struct action {
  uint16_t action;
  uint16_t outport;
};

BPF_TABLE("extern", int, struct action, action, 1);

enum {
  SLOWPATH_REASON = 1,
};

enum {
  DROP,      // drop packet
  SLOWPATH,  // send packet to user-space
  FORWARD,   // send packet through a port
};

static __always_inline int handle_rx(struct CTXTYPE *ctx,
                                     struct pkt_metadata *md) {
  pcn_log(ctx, LOG_TRACE, "Action module receiving packet");

  int i = 0;
  struct action *act = 0;
  act = action.lookup(&i);
  if (act == NULL)
    return RX_DROP;
  switch (act->action) {
  case DROP:
    pcn_log(ctx, LOG_DEBUG, "DROP");
    return RX_DROP;
  case SLOWPATH:
    pcn_log(ctx, LOG_DEBUG, "SLOWPATH");
    return pcn_pkt_controller(ctx, md, SLOWPATH_REASON);
  case FORWARD:
    pcn_log(ctx, LOG_DEBUG, "FORWARD to %d", act->outport);
    return pcn_pkt_redirect(ctx, md, act->outport);
  default:
    return RX_DROP;
  }
  return RX_DROP;
}
