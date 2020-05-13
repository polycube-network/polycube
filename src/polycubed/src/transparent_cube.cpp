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

#include "transparent_cube.h"
#include "controller.h"
#include "peer_iface.h"
#include "port.h"

#include <iostream>

// workaround for now
#include "polycubed_core.h"
extern polycube::polycubed::PolycubedCore *core;

namespace polycube {
namespace polycubed {

TransparentCube::TransparentCube(const std::string &name,
                                 const std::string &service_name,
                                 PatchPanel &patch_panel, LogLevel level,
                                 CubeType type,
                                 const service::attach_cb &attach)
    : BaseCube(name, service_name, "", patch_panel, level, type),
      ingress_next_(0),
      egress_next_(0),
      egress_next_is_netdev_(false),
      attach_(attach),
      parent_(nullptr) {}

TransparentCube::~TransparentCube() {}

void TransparentCube::uninit() {
  if (parent_) {
    parent_->remove_cube(get_name());
  }
  BaseCube::uninit();
}

std::string TransparentCube::get_wrapper_code() {
  return BaseCube::get_wrapper_code();
}

void TransparentCube::set_next(uint16_t next, ProgramType type,
                               bool is_netdev) {
  switch (type) {
  case ProgramType::INGRESS:
    if (is_netdev) {
      throw std::runtime_error("Ingress program of transparent cube can't be "
                               "followed by a netdev");
    }

    if (ingress_next_ == next) {
      return;
    }
    ingress_next_ = next;
    break;

  case ProgramType::EGRESS:
    if (egress_next_ == next && egress_next_is_netdev_ == is_netdev) {
      return;
    }
    egress_next_ = next;
    egress_next_is_netdev_ = is_netdev;
  }

  reload_all();
}

uint16_t TransparentCube::get_next(ProgramType type) {
  return type == ProgramType::INGRESS ? ingress_next_ : egress_next_;
}

void TransparentCube::set_parent(PeerIface *parent) {
  parent_ = parent;
  if (parent_) {
    attach_();

    std::lock_guard<std::mutex> lock(subscription_list_mutex);
    for (auto &it : subscription_list) {
      parent_->subscribe_parameter(uuid().str(), it.first, it.second);
    }
  }
}

PeerIface *TransparentCube::get_parent() {
  return parent_;
}

void TransparentCube::set_parameter(const std::string &parameter,
                                    const std::string &value) {
  core->set_cube_parameter(get_name(), parameter, value);
}

void TransparentCube::send_packet_out(const std::vector<uint8_t> &packet,
                                      service::Direction direction, bool recirculate) {
  Controller &c = (get_type() == CubeType::TC) ? Controller::get_tc_instance()
                                               : Controller::get_xdp_instance();

  uint16_t port = 0;
  uint16_t module;
  bool is_netdev = false;
  Port *parent_port = NULL;
  ExtIface *parent_iface = NULL;

  if (!parent_) {
      logger->error("cube doesn't have a parent.");
      return;
  }

  if (parent_port = dynamic_cast<Port *>(parent_)) {
      // calculate port
      switch (direction) {
      case service::Direction::INGRESS:
        // packet is comming in, port is ours
        port = parent_port->index();
        break;
      case service::Direction::EGRESS:
        if (parent_port->peer_port_) {
          if (dynamic_cast<ExtIface *>(parent_port->peer_port_)) {
            // If peer is an interface use parent port anyway, so it can be used
            // by the possible egress program of the parent
            port = parent_port->index();

          } else {
            // packet is going, set port to next one
            port = parent_port->peer_port_->get_port_id();
          }
        }
        break;
      }
  } else if (parent_iface = dynamic_cast<ExtIface *>(parent_)) {
    if (parent_iface->get_peer_iface()) {
      port = parent_iface->get_peer_iface()->get_port_id();
    } else {
      port = parent_iface->get_port_id();
    }
  } else {
    logger->error("cube doesn't have a valid parent.");
    return;
  }

  // calculate module index
  switch (direction) {
  case service::Direction::INGRESS:
    if (recirculate) {
      module = ingress_index_;  // myself in ingress
    } else {
      module = ingress_next_;
    }
    break;
  case service::Direction::EGRESS:
    if (recirculate) {
      module = egress_index_;  // myself in egress
    } else {
      module = egress_next_;
      is_netdev = egress_next_is_netdev_;
    }
    break;
  }

  c.send_packet_to_cube(
      module, is_netdev, port, packet, direction,
      parent_iface && direction == service::Direction::INGRESS);
}

void TransparentCube::set_conf(const nlohmann::json &conf) {
  return BaseCube::set_conf(conf);
}

nlohmann::json TransparentCube::to_json() const {
  nlohmann::json j;
  j.update(BaseCube::to_json());

  std::string parent;
  if (parent_) {
    if (auto port_parent = dynamic_cast<Port *>(parent_)) {
      parent = port_parent->get_path();
    } else if (auto iface_parent = dynamic_cast<ExtIface *>(parent_)) {
      parent = iface_parent->get_iface_name();
    }
  }

  j["parent"] = parent;

  return j;
}

void TransparentCube::subscribe_parent_parameter(
    const std::string &param_name, ParameterEventCallback &callback) {

  std::lock_guard<std::mutex> lock(subscription_list_mutex);
  // Add event to the list
  subscription_list.emplace(param_name, callback);

  // If not parent, just return, the subcription will done when the cube is attached
  if (!parent_) {
    return;
  }
  parent_->subscribe_parameter(uuid().str(), param_name, callback);
}

void TransparentCube::unsubscribe_parent_parameter(
    const std::string &param_name) {

  std::lock_guard<std::mutex> lock(subscription_list_mutex);
  // Remove event from the list
  subscription_list.erase(param_name);

  // If not parent, just return
  if (!parent_) {
    return;
  }
  parent_->unsubscribe_parameter(uuid().str(), param_name);
}

std::string TransparentCube::get_parent_parameter(
    const std::string &param_name) {
  if (!parent_) {
    throw std::runtime_error("cube is not attached");
  }

  return parent_->get_parameter(param_name);
}

}  // namespace polycubed
}  // namespace polycube
