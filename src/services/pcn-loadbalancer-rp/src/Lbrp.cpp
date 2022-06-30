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
using namespace polycube::service;

const std::string Lbrp::EBPF_IP_TO_FRONTEND_PORT_MAP = "ip_to_frontend_port";

Lbrp::Lbrp(const std::string name, const LbrpJsonObject &conf)
    : Cube(conf.getBase(), {Lbrp::buildLbrpCode(lbrp_code, conf.getPortMode())},
           {}), lbrp_code_{Lbrp::buildLbrpCode(lbrp_code, conf.getPortMode())},
      port_mode_{conf.getPortMode()} {
  logger()->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [Lbrp] [%n] [%l] %v");
  logger()->info("Creating Lbrp instance in {0} port mode",
                 LbrpJsonObject::LbrpPortModeEnum_to_string(port_mode_));

  addServiceList(conf.getService());
  addSrcIpRewrite(conf.getSrcIpRewrite());
  addPortsList(conf.getPorts());
}

Lbrp::~Lbrp() {
  logger()->info("Destroying Lbrp instance");
}

void Lbrp::update(const LbrpJsonObject &conf) {
  // This method updates all the object/parameter in Lbrp object specified in
  // the conf JsonObject.
  // You can modify this implementation.
  Cube::set_conf(conf.getBase());

  if (conf.serviceIsSet()) {
    for (auto &i : conf.getService()) {
      auto vip = i.getVip();
      auto vport = i.getVport();
      auto proto = i.getProto();
      auto m = getService(vip, vport, proto);
      m->update(i);
    }
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
  conf.setBase(Cube::to_json());

  for (auto &i : getServiceList()) {
    conf.addService(i->toJsonObject());
  }

  conf.setSrcIpRewrite(getSrcIpRewrite()->toJsonObject());

  for (auto &i : getPortsList()) {
    conf.addPorts(i->toJsonObject());
  }

  return conf;
}

std::string Lbrp::buildLbrpCode(std::string const &lbrp_code,
                                LbrpPortModeEnum port_mode) {
  if (port_mode == LbrpPortModeEnum::SINGLE) {
    return "#define SINGLE_PORT_MODE 1\n" + lbrp_code;
  }
  return "#define SINGLE_PORT_MODE 0\n" + lbrp_code;
}

void Lbrp::flood_packet(Ports &port, PacketInMetadata &md,
                           const std::vector<uint8_t> &packet) {
  EthernetII p(&packet[0], packet.size());

  for (auto &it: get_ports()) {
    if (it->name() == port.name()) {
      continue;
    }
    it->send_packet_out(p);
    logger()->trace("Sent pkt to port {0} as result of flooding", it->name());
  }
}

void Lbrp::packet_in(Ports &port, polycube::service::PacketInMetadata &md,
                        const std::vector<uint8_t> &packet) {
  try {
    switch (static_cast<SlowPathReason>(md.reason)) {
      case SlowPathReason::ARP_REPLY: {
        logger()->debug("Received pkt in slowpath - reason: ARP_REPLY");
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
        break;
      }
      case SlowPathReason::FLOODING: {
        logger()->debug("Received pkt in slowpath - reason: FLOODING");
        flood_packet(port, md, packet);
        break;
      }
      default: {
        logger()->error("Not valid reason {0} received", md.reason);
      }
    }
  } catch (const std::exception &e) {
    logger()->error("Exception during slowpath packet processing: '{0}'",
                    e.what());
  }
}

void Lbrp::reloadCodeWithNewPorts() {
  uint16_t frontend_port = 0;
  uint16_t backend_port = 1;

  for (auto &it: get_ports()) {
    if (it->getType() == PortsTypeEnum::FRONTEND) {
      frontend_port = it->index();
    } else backend_port = it->index();
  }

  logger()->debug("Reloading code with FRONTEND port {0} and BACKEND port {1}",
                  frontend_port, backend_port);
  std::string frontend_port_str("#define FRONTEND_PORT " +
                                std::to_string(frontend_port));
  std::string backend_port_str("#define BACKEND_PORT " +
                               std::to_string(backend_port));

  reload(frontend_port_str + "\n" + backend_port_str + "\n" + lbrp_code_);

  logger()->trace("New lbrp code loaded");
}

void Lbrp::reloadCodeWithNewBackendPort(uint16_t backend_port_index) {
  logger()->debug("Reloading code with BACKEND port {0}", backend_port_index);
  std::string backend_port_str("#define BACKEND_PORT " +
                               std::to_string(backend_port_index));

  reload(backend_port_str + "\n" + lbrp_code_);

  logger()->trace("New lbrp code loaded");
}

std::vector<std::shared_ptr<Ports>> Lbrp::getFrontendPorts() {
  std::vector<std::shared_ptr<Ports>> frontend_ports;
  for (auto &it: get_ports()) {
    if (it->getType() == PortsTypeEnum::FRONTEND) {
      frontend_ports.push_back(it);
    }
  }
  return frontend_ports;
}

std::shared_ptr<Ports> Lbrp::getBackendPort() {
  for (auto &it : get_ports()) {
    if (it->getType() == PortsTypeEnum::BACKEND) {
      return it;
    }
  }
  return nullptr;
}

std::shared_ptr<Ports> Lbrp::getPorts(const std::string &name) {
  return get_port(name);
}

std::vector<std::shared_ptr<Ports>> Lbrp::getPortsList() {
  return get_ports();
}

void Lbrp::addPortsInSinglePortMode(const std::string &name,
                                    const PortsJsonObject &conf) {
  PortsTypeEnum type = conf.getType();
  auto ip_is_set = conf.ipIsSet();
  if (get_ports().size() == 2) {
    throw std::runtime_error("Reached maximum number of ports");
  }

  if (type == PortsTypeEnum::FRONTEND) {
    if (getFrontendPorts().size() == 1) {
      throw std::runtime_error("There is already a FRONTEND port. Only a "
                               "FRONTEND port is allowed in SINGLE port mode");
    }
    if (ip_is_set) {
      throw std::runtime_error("The ip address in not allow in SINGLE port "
                               "mode for a FRONTEND port");
    }
  } else {
    if (getBackendPort() != nullptr) throw std::runtime_error("There is "
                               "already a BACKEND port");
    if (ip_is_set) throw std::runtime_error("The ip address in not allowed "
                               "in BACKEND port");
  }
  add_port<PortsJsonObject>(name, conf);
  if (get_ports().size() == 2) reloadCodeWithNewPorts();
}

void Lbrp::addPortsInMultiPortMode(const std::string &name,
                                   const PortsJsonObject &conf) {
  PortsTypeEnum type = conf.getType();
  auto ip_is_set = conf.ipIsSet();

  if (type == PortsTypeEnum::FRONTEND) {
    if (!ip_is_set) {
      throw std::runtime_error("The IP address is mandatory in MULTI port "
                               "mode for a FRONTEND port");
    }
    auto ip = conf.getIp();
    auto found = frontend_ip_set_.find(ip) != frontend_ip_set_.end();
    if (found) {
      throw std::runtime_error("A FRONTEND port with the provided IP "
                               "address (" + ip + ") already exist");
    }
    if (!frontend_ip_set_.insert(ip).second) {
      throw std::runtime_error("Failed to set the IP address for the new "
                               "FRONTEND port");
    }
    try {
      auto created_port = add_port<PortsJsonObject>(name, conf);
      auto ip_to_frontend_port_table = get_hash_table<uint32_t, uint16_t>(
          EBPF_IP_TO_FRONTEND_PORT_MAP);
      ip_to_frontend_port_table.set(
          utils::ip_string_to_nbo_uint(created_port->getIp()),
          created_port->index()
      );
    } catch (std::exception &ex) {
      frontend_ip_set_.erase(ip);
      throw;
    }
  } else {
    if (getBackendPort() != nullptr) {
      throw std::runtime_error("There is already a BACKEND port");
    }
    if (ip_is_set) {
      throw std::runtime_error("The ip address in not allowed in BACKEND port");
    }
    auto created_port = add_port<PortsJsonObject>(name, conf);
    reloadCodeWithNewBackendPort(created_port->index());
  }
}

void Lbrp::addPorts(const std::string &name, const PortsJsonObject &conf) {
  try {
    if (port_mode_ == LbrpPortModeEnum::SINGLE) {
      addPortsInSinglePortMode(name, conf);
    }
    else addPortsInMultiPortMode(name, conf);
  } catch (std::runtime_error &ex) {
    logger()->warn("Failed to add port {0}: {1}", name, ex.what());
    throw;
  }

  logger()->info("Created new Port with name {0}", name);
}

void Lbrp::addPortsList(const std::vector<PortsJsonObject> &conf) {
  for (auto &i : conf) {
    std::string name_ = i.getName();
    addPorts(name_, i);
  }
}

void Lbrp::replacePorts(const std::string &name, const PortsJsonObject &conf) {
  delPorts(name);
  std::string name_ = conf.getName();
  addPorts(name_, conf);
}

void Lbrp::delPorts(const std::string &name) {
  try {
    auto port = get_port(name);
    if (this->port_mode_ == LbrpPortModeEnum::MULTI &&
        port->getType() == PortsTypeEnum::FRONTEND) {
      auto ip = port->getIp();

      auto ip_to_frontend_port_table = get_hash_table<uint32_t, uint16_t>(
          EBPF_IP_TO_FRONTEND_PORT_MAP);
      ip_to_frontend_port_table.remove(utils::ip_string_to_nbo_uint(ip));

      if (this->frontend_ip_set_.erase(ip) == 0) {
        throw std::runtime_error(
            "Failed to delete the port IP address for the new FRONTEND port");
      }
    }
    remove_port(name);
  } catch (std::exception &ex) {
    logger()->error("Failed to delete port {0}: {1}", name, ex.what());
    throw;
  }
  logger()->info("Deleted Lbrp port");
}

void Lbrp::delPortsList() {
  auto ports = get_ports();
  for (auto it : ports) {
    delPorts(it->name());
  }
}

LbrpPortModeEnum Lbrp::getPortMode() {
  return port_mode_;
}

void Lbrp::setPortMode(const LbrpPortModeEnum &value) {
  logger()->warn("Port mode cannot be changed at runtime");
  throw std::runtime_error("Port mode cannot be changed at runtime");
}

std::shared_ptr<SrcIpRewrite> Lbrp::getSrcIpRewrite() {
  return src_ip_rewrite_;
}

void Lbrp::addSrcIpRewrite(const SrcIpRewriteJsonObject &value) {
  logger()->debug(
      "[SrcIpRewrite] Received request to create SrcIpRewrite range {0} , new "
      "ip range {1} ",
      value.getIpRange(), value.getNewIpRange());

  src_ip_rewrite_ = std::make_shared<SrcIpRewrite>(*this, value);
  src_ip_rewrite_->update(value);
}

void Lbrp::replaceSrcIpRewrite(const SrcIpRewriteJsonObject &conf) {
  delSrcIpRewrite();
  addSrcIpRewrite(conf);
}

void Lbrp::delSrcIpRewrite() {
  // what the hell means to remove entry in this case?
}

std::shared_ptr<Service> Lbrp::getService(const std::string &vip,
                                          const uint16_t &vport,
                                          const ServiceProtoEnum &proto) {
  // This method retrieves the pointer to Service object specified by its keys.
  logger()->debug("[Service] Received request to read a service entry");
  logger()->debug("[Service] Virtual IP: {0}, virtual port: {1}, protocol: {2}",
                  vip, vport,
                  ServiceJsonObject::ServiceProtoEnum_to_string(proto));

  uint16_t vp = vport;
  if (Service::convertProtoToNumber(proto) == 1)
    vp = vport;

  Service::ServiceKey key =
      Service::ServiceKey(vip, vp, Service::convertProtoToNumber(proto));

  if (service_map_.count(key) == 0) {
    logger()->error("[Service] There are no entries associated with that key");
    throw std::runtime_error("There are no entries associated with that key");
  }

  return std::shared_ptr<Service>(&service_map_.at(key), [](Service *) {});
}

std::vector<std::shared_ptr<Service>> Lbrp::getServiceList() {
  std::vector<std::shared_ptr<Service>> services_vect;

  for (auto &it : service_map_) {
    Service::ServiceKey key = it.first;
    services_vect.push_back(
        getService(std::get<0>(key), std::get<1>(key),
                   Service::convertNumberToProto(std::get<2>(key))));
  }

  return services_vect;
}

void Lbrp::addService(const std::string &vip, const uint16_t &vport,
                      const ServiceProtoEnum &proto,
                      const ServiceJsonObject &conf) {
  logger()->debug("[Service] Received request to create new service entry");
  logger()->debug("[Service] Virtual IP: {0}, virtual port: {1}, protocol: {2}",
                  vip, vport,
                  ServiceJsonObject::ServiceProtoEnum_to_string(proto));

  if (proto == ServiceProtoEnum::ALL) {
    // Let's create 3 different services for TCP, UDP and ICMP
    // Let's start from TCP
    ServiceJsonObject new_conf(conf);
    new_conf.setProto(ServiceProtoEnum::TCP);
    addService(vip, vport, ServiceProtoEnum::TCP, new_conf);

    // Now is the UDP turn
    new_conf.setProto(ServiceProtoEnum::UDP);
    addService(vip, vport, ServiceProtoEnum::UDP, new_conf);

    // Finally, is ICMP turn. For ICMP we use a "special" port since this field
    // is useless
    new_conf.setProto(ServiceProtoEnum::ICMP);
    new_conf.setVport(Service::ICMP_EBPF_PORT);
    addService(vip, Service::ICMP_EBPF_PORT, ServiceProtoEnum::ICMP, new_conf);

    // We completed all task, let's directly return since we do not have to
    // execute the code below
    return;
  } else if (proto == ServiceProtoEnum::ICMP &&
             conf.getVport() != Service::ICMP_EBPF_PORT) {
    throw std::runtime_error(
        "ICMP Service requires 0 as virtual port. Since this parameter is "
        "useless for ICMP services");
  }
  
  Service::ServiceKey key =
      Service::ServiceKey(vip, vport, Service::convertProtoToNumber(proto));
  if (service_map_.count(key) != 0) {
    logger()->error("[Service] This service already exists");
    throw std::runtime_error("This service already exists");
  }
  Service service = Service(*this,conf);
  service_map_.insert(std::make_pair(key,service));

}

void Lbrp::addServiceList(const std::vector<ServiceJsonObject> &conf) {
  for (auto &i : conf) {
    std::string vip_ = i.getVip();
    uint16_t vport_ = i.getVport();
    ServiceProtoEnum proto_ = i.getProto();
    addService(vip_, vport_, proto_, i);
  }
}

void Lbrp::replaceService(const std::string &vip, const uint16_t &vport,
                          const ServiceProtoEnum &proto,
                          const ServiceJsonObject &conf) {
  delService(vip, vport, proto);
  std::string vip_ = conf.getVip();
  uint16_t vport_ = conf.getVport();
  ServiceProtoEnum proto_ = conf.getProto();
  addService(vip_, vport_, proto_, conf);
}

void Lbrp::delService(const std::string &vip, const uint16_t &vport,
                      const ServiceProtoEnum &proto) {
  if (proto == ServiceProtoEnum::ALL) {
    // Let's create 3 different services for TCP, UDP and ICMP
    // Let's start from TCP
    delService(vip, vport, ServiceProtoEnum::TCP);

    // Now is the UDP turn
    delService(vip, vport, ServiceProtoEnum::UDP);

    // Finally, is ICMP turn. For ICMP we use a "special" port since this field
    // is useless
    delService(vip, Service::ICMP_EBPF_PORT, ServiceProtoEnum::ICMP);

    // We completed all task, let's directly return since we do not have to
    // execute the code below
    return;
  } else if (proto == ServiceProtoEnum::ICMP &&
             vport != Service::ICMP_EBPF_PORT) {
    throw std::runtime_error(
        "ICMP Service requires 0 as virtual port. Since this parameter is "
        "useless for ICMP services");
  }

  logger()->debug("[Service] Received request to delete a service entry");
  logger()->debug("[Service] Virtual IP: {0}, virtual port: {1}, protocol: {2}",
                  vip, vport,
                  ServiceJsonObject::ServiceProtoEnum_to_string(proto));

  Service::ServiceKey key =
      Service::ServiceKey(vip, vport, Service::convertProtoToNumber(proto));

  if (service_map_.count(key) == 0) {
    logger()->error("[Service] There are no entries associated with that key");
    throw std::runtime_error("There are no entries associated with that key");
  }

  auto service = getService(vip, vport, proto);

  service->delBackendList();
  service_map_.erase(key);
}

void Lbrp::delServiceList() {
  for (auto it = service_map_.begin(); it != service_map_.end();) {
    auto tmp = it;
    it++;
    delService(tmp->second.getVip(), tmp->second.getVport(),
               tmp->second.getProto());
  }
}