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

#include "polycube/services/transparent_cube.h"

#include <tins/ethernetII.h>

namespace polycube {
namespace service {

TransparentCube::TransparentCube(const nlohmann::json &conf,
                                 const std::vector<std::string> &ingress_code,
                                 const std::vector<std::string> &egress_code)
    : BaseCube(conf, ingress_code, egress_code) {
  handle_packet_in = [&](const PacketIn *md,
                         const std::vector<uint8_t> &packet) -> void {
    // This lock guarantees:
    // - service implementation is not deleted wile processing it
    std::lock_guard<std::mutex> guard(cube_mutex);
    if (dismounted_)
      return;

    Direction direction = static_cast<Direction>(md->port_id);
    PacketInMetadata md_;
    md_.reason = md->reason;
    md_.metadata[0] = md->metadata[0];
    md_.metadata[1] = md->metadata[1];
    md_.metadata[2] = md->metadata[2];
    packet_in(direction, md_, packet);
  };

  cube_ = factory_->create_transparent_cube(
      conf, ingress_code, egress_code, handle_log_msg,
      std::bind(&TransparentCube::set_control_plane_log_level, this,
                std::placeholders::_1),
      handle_packet_in, std::bind(&TransparentCube::attach, this));
  // TODO: where to keep this reference?, keep a double reference?
  BaseCube::cube_ = cube_;
}

TransparentCube::~TransparentCube() {
  // just in case
  dismount();

  handle_packet_in = nullptr;

  factory_->destroy_cube(get_name());
}

void TransparentCube::send_packet_out(EthernetII &packet, Direction direction,
                                      bool recirculate) {
  cube_->send_packet_out(packet.serialize(), direction, recirculate);
}

void TransparentCube::set_conf(const nlohmann::json &conf) {
  cube_->set_conf(conf);
}

nlohmann::json TransparentCube::to_json() const  {
  return cube_->to_json();
}

void TransparentCube::attach() {}

void TransparentCube::subscribe_parent_parameter(
    const std::string &param_name, ParameterEventCallback &callback) {
  cube_->subscribe_parent_parameter(param_name, callback);
}

void TransparentCube::unsubscribe_parent_parameter(
    const std::string &param_name) {
  cube_->unsubscribe_parent_parameter(param_name);
}

std::string TransparentCube::get_parent_parameter(
    const std::string &param_name) {
  return cube_->get_parent_parameter(param_name);
}

}  // namespace service
}  // namespace polycube
