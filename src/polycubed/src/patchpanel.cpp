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

#include "patchpanel.h"

#include <iostream>

namespace polycube {
namespace polycubed {

const std::string PatchPanel::PATCHPANEL_CODE = R"(
BPF_TABLE_PUBLIC("prog", int, int, _MAP_NAME, _POLYCUBE_MAX_NODES);
)";

PatchPanel &PatchPanel::get_tc_instance() {
  static PatchPanel tc_instance("nodes", Node::_POLYCUBE_MAX_NODES);
  return tc_instance;
}

PatchPanel &PatchPanel::get_xdp_instance() {
  static PatchPanel xdp_instance("xdp_nodes", Node::_POLYCUBE_MAX_NODES);
  return xdp_instance;
}

PatchPanel::PatchPanel(const std::string &map_name, int max_nodes)
    : max_nodes_(max_nodes), logger(spdlog::get("polycubed")) {
  std::vector<std::string> flags;
  //flags.push_back(std::string("-DMAP_NAME=") + map_name);
  flags.push_back(std::string("-D_POLYCUBE_MAX_NODES=") +
                  std::to_string(max_nodes_));
  std::string code(PATCHPANEL_CODE);
  code.replace(code.find("_MAP_NAME"), 9, map_name);

  auto init_res = program_.init(code, flags);
  if (init_res.code() != 0) {
    logger->error("error creating patch panel: {0}", init_res.msg());
    throw std::runtime_error("Error creating patch panel");
  }

  for (uint16_t i = 1; i < max_nodes_; i++)
    node_ids_.insert(i);

  // TODO: is this code valid?
  // (implicit copy constructor should be ok for this case)
  auto a = program_.get_prog_table(map_name);
  nodes_ = std::unique_ptr<ebpf::BPFProgTable>(new ebpf::BPFProgTable(a));
}

PatchPanel::~PatchPanel() {}

void PatchPanel::add(Node &n) {
  // FIXME: what happens if all ports are busy?
  int p = *node_ids_.begin();
  node_ids_.erase(p);
  nodes_->update_value(p, n.get_fd());
  n.set_index(p);
}

void PatchPanel::add(Node &n, uint16_t index) {
  if (node_ids_.count(index) == 0) {
    logger->error("index '{0}' is busy in patch panel", index);
    throw std::runtime_error("Index is busy");
  }

  node_ids_.erase(index);
  nodes_->update_value(index, n.get_fd());
  n.set_index(index);
}

uint16_t PatchPanel::add(int fd) {
  int p = *node_ids_.begin();
  node_ids_.erase(p);
  nodes_->update_value(p, fd);
  return p;
}

void PatchPanel::remove(Node &n) {
  int index = n.get_index();
  node_ids_.insert(index);
  nodes_->remove_value(index);
}

void PatchPanel::remove(uint16_t index) {
  node_ids_.insert(index);
  nodes_->remove_value(index);
}

void PatchPanel::update(Node &n) {
  if (node_ids_.count(n.get_index()) != 0) {
    logger->error("index '{0}' is not registered", n.get_index());
    throw std::runtime_error("Index is not registered");
  }

  nodes_->update_value(n.get_index(), n.get_fd());
}

void PatchPanel::update(uint16_t index, int fd) {
  if (node_ids_.count(index) != 0) {
    logger->error("index '{0}' is not registered", index);
    throw std::runtime_error("Index is not registered");
  }

  nodes_->update_value(index, fd);
}

}  // namespace polycubed
}  // namespace polycube
