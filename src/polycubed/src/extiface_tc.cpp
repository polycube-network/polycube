/*
 * Copyright 2016 PLUMgrid
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

#include "extiface_tc.h"
#include "bcc_mutex.h"
#include "exceptions.h"
#include "patchpanel.h"
#include "utils/netlink.h"

#include <iostream>

namespace polycube {
namespace polycubed {

ExtIfaceTC::ExtIfaceTC(const std::string &iface) : ExtIface(iface) {
  try {
    std::unique_lock<std::mutex> bcc_guard(bcc_mutex);
    int fd_rx = load_ingress();
    int fd_tx = load_tx();
    load_egress();

    index_ = PatchPanel::get_tc_instance().add(fd_tx);
    Netlink::getInstance().attach_to_tc(iface_, fd_rx);
  } catch (...) {
    used_ifaces.erase(iface);
    throw;
  }
}

ExtIfaceTC::~ExtIfaceTC() {
  PatchPanel::get_tc_instance().remove(index_);
  Netlink::getInstance().detach_from_tc(iface_);
}

std::string ExtIfaceTC::get_ingress_code() const {
  return RX_CODE;
}

std::string ExtIfaceTC::get_egress_code() const {
  return RX_CODE;
}

std::string ExtIfaceTC::get_tx_code() const {
  return TX_CODE;
}

bpf_prog_type ExtIfaceTC::get_program_type() const {
  return BPF_PROG_TYPE_SCHED_CLS;
}

const std::string ExtIfaceTC::RX_CODE = R"(
#include <uapi/linux/pkt_cls.h>

BPF_TABLE("extern", int, int, nodes, _POLYCUBE_MAX_NODES);
// TODO: how does it impact the performance?
BPF_TABLE("array", int, u32, index_map, 1);

int handler(struct __sk_buff *skb) {
  //bpf_trace_printk("ingress %d %x\n", skb->ifindex, INDEX);

  int zero = 0;
  u32 *index = index_map.lookup(&zero);
  if (!index)
    return TC_ACT_OK;

  skb->cb[0] = *index;
  nodes.call(skb, *index & 0xFFFF);

  //bpf_trace_printk("miss tailcall\n");
  return TC_ACT_OK;
}
)";

const std::string ExtIfaceTC::TX_CODE = R"(
#include <uapi/linux/pkt_cls.h>
int handler(struct __sk_buff *skb) {
  //bpf_trace_printk("egress %d\n", INTERFACE_INDEX);
  bpf_redirect(INTERFACE_INDEX, 0);
  return TC_ACT_REDIRECT;
}
)";

}  // namespace polycubed
}  // namespace polycube
