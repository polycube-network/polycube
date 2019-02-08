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

// Modify these methods with your own implementation

#include "Lbrp.h"
#include "Lbrp_dp.h"

#include <tins/ethernetII.h>
#include <tins/tins.h>

using namespace Tins;

Lbrp::Lbrp(const std::string name, const LbrpJsonObject &conf, CubeType type)
    : Cube(name, {generate_code()}, {}, type, conf.getPolycubeLoglevel()) {
  logger()->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [Lbrp] [%n] [%l] %v");
  logger()->info("Creating Lbrp instance");

  addServiceList(conf.getService());
  addSrcIpRewrite(conf.getSrcIpRewrite());
  addPortsList(conf.getPorts());
}

Lbrp::~Lbrp() {}

void Lbrp::update(const LbrpJsonObject &conf) {
  // This method updates all the object/parameter in Lbrp object specified in
  // the conf JsonObject.
  // You can modify this implementation.

  if (conf.serviceIsSet()) {
    for (auto &i : conf.getService()) {
      auto vip = i.getVip();
      auto vport = i.getVport();
      auto proto = i.getProto();
      auto m = getService(vip, vport, proto);
      m->update(i);
    }
  }

  if (conf.loglevelIsSet()) {
    setLoglevel(conf.getLoglevel());
  }

  if (conf.srcIpRewriteIsSet()) {
    auto m = getSrcIpRewrite();
    m->update(conf.getSrcIpRewrite());
  }

  if (conf.portsIsSet()) {
    for (auto &i : conf.getPorts()) {
      auto name = i.getName();
      auto m = getPorts(name);
      m->update(i);
    }
  }
}

LbrpJsonObject Lbrp::toJsonObject() {
  LbrpJsonObject conf;

  conf.setName(getName());

  for (auto &i : getServiceList()) {
    conf.addService(i->toJsonObject());
  }

  conf.setLoglevel(getLoglevel());
  conf.setSrcIpRewrite(getSrcIpRewrite()->toJsonObject());
  conf.setType(getType());

  for (auto &i : getPortsList()) {
    conf.addPorts(i->toJsonObject());
  }

  conf.setUuid(getUuid());
  return conf;
}

std::string Lbrp::generate_code() {
  return lbrp_code;
}

std::vector<std::string> Lbrp::generate_code_vector() {
  throw std::runtime_error("Method not implemented");
}

void Lbrp::packet_in(Ports &port, polycube::service::PacketInMetadata &md,
                     const std::vector<uint8_t> &packet) {
  EthernetII eth(&packet[0], packet.size());

  auto b_port = getBackendPort();
  if (!b_port)
    return;

  // send the original packet
  logger()->debug("Sending original ARP reply to backend port: {}",
                  b_port->name());
  b_port->send_packet_out(eth);

  // send a second packet for the virtual src IPs
  ARP &arp = eth.rfind_pdu<ARP>();

  uint32_t ip = uint32_t(arp.sender_ip_addr());
  ip &= htonl(src_ip_rewrite_->mask);
  ip |= htonl(src_ip_rewrite_->net);

  IPv4Address addr(ip);
  arp.sender_ip_addr(addr);

  logger()->debug("Sending modified ARP reply to backend port: {}",
                  b_port->name());
  b_port->send_packet_out(eth);
}

void Lbrp::reloadCodeWithNewPorts() {
  uint16_t frontend_port = 0;
  uint16_t backend_port = 1;

  for (auto &it : get_ports()) {
    switch (it->getType()) {
    case PortsTypeEnum::FRONTEND:
      frontend_port = it->index();
      break;
    case PortsTypeEnum::BACKEND:
      backend_port = it->index();
      break;
    }
  }

  logger()->debug("Reloading code with frontend port: {0} and backend port {1}",
                  frontend_port, backend_port);
  std::string frontend_port_str("#define FRONTEND_PORT " +
                                std::to_string(frontend_port));
  std::string backend_port_str("#define BACKEND_PORT " +
                               std::to_string(backend_port));

  reload(frontend_port_str + "\n" + backend_port_str + "\n" + lbrp_code);

  logger()->trace("New lbrp code loaded");
}

std::shared_ptr<Ports> Lbrp::getFrontendPort() {
  for (auto &it : get_ports()) {
    if (it->getType() == PortsTypeEnum::FRONTEND) {
      return it;
    }
  }
  return nullptr;
}

std::shared_ptr<Ports> Lbrp::getBackendPort() {
  for (auto &it : get_ports()) {
    if (it->getType() == PortsTypeEnum::BACKEND) {
      return it;
    }
  }
  return nullptr;
}
