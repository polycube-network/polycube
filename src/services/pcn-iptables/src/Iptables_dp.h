/*
 * Copyright 2018 The Polycube Authors
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

#pragma once

#include <string>

const std::string iptables_code_ingress = R"POLYCUBE_DP(
/*
 * This function is called each time a packet arrives to the cube.
 * ctx contains the packet and md some additional metadata for the packet.
 * If the service is of type XDP_SKB/DRV CTXTYPE is equivalent to the struct xdp_md
 * otherwise, if the service is of type TC, CTXTYPE is equivalent to the __sk_buff struct
 * Please look at the libpolycube documentation for more details.
 */
static __always_inline
int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  // Put your eBPF datapth code here
  pcn_log(ctx, LOG_INFO, "Hello from pcn-iptables datapath INGRESS! :-)");
  return RX_OK;
}
)POLYCUBE_DP";

const std::string iptables_code_egress = R"POLYCUBE_DP(
/*
 * This function is called each time a packet arrives to the cube.
 * ctx contains the packet and md some additional metadata for the packet.
 * If the service is of type XDP_SKB/DRV CTXTYPE is equivalent to the struct xdp_md
 * otherwise, if the service is of type TC, CTXTYPE is equivalent to the __sk_buff struct
 * Please look at the libpolycube documentation for more details.
 */
static __always_inline
int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  // Put your eBPF datapth code here
  pcn_log(ctx, LOG_INFO, "Hello from pcn-iptables datapath EGRESS! :-)");
  return RX_OK;
}
)POLYCUBE_DP";
