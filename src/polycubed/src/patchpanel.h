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

#pragma once

#include "node.h"

#include <set>

#include <api/BPF.h>

#include <spdlog/spdlog.h>

namespace polycube {
namespace polycubed {
/*
 * PatchPanel is the place where all the nodes_ present are plugged together.
 * in eBPF idiom it is a prog map where file-descriptors to all eBPF programs
 * are saved, them Cubes could do tail calls to implement chains.
 */
class PatchPanel {
 public:
  static PatchPanel &get_tc_instance();
  static PatchPanel &get_xdp_instance();
  ~PatchPanel();
  void add(Node &n);
  void add(Node &n, uint16_t index);  // add node at specific position
  uint16_t add(int fd);               // returns the index
  void remove(Node &n);
  void remove(uint16_t index);
  void update(Node &n);
  void update(uint16_t index, int fd);

 private:
  PatchPanel(const std::string &map_name, int max_nodes);
  ebpf::BPF program_;
  std::set<uint16_t> node_ids_;
  std::unique_ptr<ebpf::BPFProgTable> nodes_;
  static const std::string PATCHPANEL_CODE;
  int max_nodes_;

  std::shared_ptr<spdlog::logger> logger;
};

}  // namespace polycubed
}  // namespace polycube
