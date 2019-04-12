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

/*
 * This file contains the eBPF code that implements the service datapath.
 * Of course it is no required to have this into a separated file, however
 * it could be a good idea in order to better organize the code.
 */

#include <bcc/helpers.h>
#include <bcc/proto.h>

const uint16_t UINT16_MAX = 0xffff;

enum {
  SLOWPATH_REASON = 1,
};

enum {
  DROP,      // drop packet
  SLOWPATH,  // send packet to user-space
  FORWARD,   // forward packet between ports
};

/*
 * BPF map of single element that saves the action to be applied in packets
 */
BPF_ARRAY(action_map, uint8_t, 1);

/*
 * BPF map where the ids of the ports are saved.  This module supports at most
 * two ports
 */
BPF_ARRAY(ports_map, uint16_t, 2);

/*
 * This function is called each time a packet arrives to the cube.
 * ctx contains the packet and md some additional metadata for the packet.
 * If the service is of type XDP_SKB/DRV CTXTYPE is equivalent to the struct
 * xdp_md
 * otherwise, if the service is of type TC, CTXTYPE is equivalent to the
 * __sk_buff struct
 * Please look at the polycube documentation for more details.
 */
static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  pcn_log(ctx, LOG_DEBUG, "Receiving packet from port %d", md->in_port);
  pcn_pkt_log(ctx, LOG_DEBUG);

  unsigned int zero = 0;
  unsigned int one = 1;

  uint8_t *action = action_map.lookup(&zero);
  if (!action) {
    return RX_DROP;
  }

  // what action should be performed in the packet?
  switch (*action) {
  case DROP:
    pcn_log(ctx, LOG_DEBUG, "Dropping packet");
    return RX_DROP;
  case SLOWPATH:
    pcn_log(ctx, LOG_DEBUG, "Sending packet to slow path");
    return pcn_pkt_controller(ctx, md, SLOWPATH_REASON);
  case FORWARD: ;
    // Get ports ids
    uint16_t *p1 = ports_map.lookup(&zero);
    if (!p1 || *p1 == UINT16_MAX) {
      return RX_DROP;
    }

    uint16_t *p2 = ports_map.lookup(&one);
    if (!p2 || *p2 == UINT16_MAX) {
      return RX_DROP;
    }

    pcn_log(ctx, LOG_DEBUG, "Forwarding packet");

    uint16_t outport = md->in_port == *p1 ? *p2 : *p1;
    return pcn_pkt_redirect(ctx, md, outport);
  default:
    // if control plane is well implemented this will never happen
    pcn_log(ctx, LOG_ERR, "bad action %d", *action);
    return RX_DROP;
  }
}
