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

Lbrp::Lbrp(const std::string name, const LbrpJsonObject &conf)
    : Cube(conf.getBase(), {lbrp_code}, {}) {
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

std::shared_ptr<Ports> Lbrp::getPorts(const std::string &name) {
  return get_port(name);
}

std::vector<std::shared_ptr<Ports>> Lbrp::getPortsList() {
  return get_ports();
}

void Lbrp::addPorts(const std::string &name, const PortsJsonObject &conf) {
  if (get_ports().size() == 2) {
    logger()->warn("Reached maximum number of ports");
    throw std::runtime_error("Reached maximum number of ports");
  }

  try {
    switch (conf.getType()) {
    case PortsTypeEnum::FRONTEND:
      if (getFrontendPort() != nullptr) {
        logger()->warn("There is already a FRONTEND port");
        throw std::runtime_error("There is already a FRONTEND port");
      }
      break;
    case PortsTypeEnum::BACKEND:
      if (getBackendPort() != nullptr) {
        logger()->warn("There is already a BACKEND port");
        throw std::runtime_error("There is already a BACKEND port");
      }
      break;
    }
  } catch (std::runtime_error &e) {
    logger()->warn("Error when adding the port {0}", name);
    logger()->warn("Error message: {0}", e.what());
    throw;
  }

  add_port<PortsJsonObject>(name, conf);

  if (get_ports().size() == 2) {
    logger()->info("Reloading code because of the new port");
    reloadCodeWithNewPorts();
  }

  logger()->info("New port created with name {0}", name);
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
  remove_port(name);
}

void Lbrp::delPortsList() {
  auto ports = get_ports();
  for (auto it : ports) {
    delPorts(it->name());
  }
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

  service->removeServiceFromKernelMap();
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