/*
 * Copyright 2021 Leonardo Di Giovanna
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

#include "K8slbrp.h"
#include "K8slbrp_dp.h"

#include <tins/ethernetII.h>
#include <tins/tins.h>

using namespace Tins;
using namespace polycube::service;


const std::string K8slbrp::EBPF_IP_TO_FRONTEND_PORT_MAP = "ip_to_frontend_port";

K8slbrp::K8slbrp(const std::string name, const K8slbrpJsonObject &conf) :
        Cube(conf.getBase(), {K8slbrp::buildK8sLbrpCode(k8slbrp_code, conf.getPortMode())}, {}),
        K8slbrpBase(name),
        k8slbrp_code_{K8slbrp::buildK8sLbrpCode(k8slbrp_code, conf.getPortMode())},
        port_mode_{conf.getPortMode()} {
    logger()->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [K8slbrp] [%n] [%l] %v");
    logger()->info(
            "creating K8slbrp instance in {0} port mode",
            K8slbrpJsonObject::K8slbrpPortModeEnum_to_string(port_mode_)
    );

    addServiceList(conf.getService());
    addSrcIpRewrite(conf.getSrcIpRewrite());
    addPortsList(conf.getPorts());
}

K8slbrp::~K8slbrp() {
    logger()->info("destroying K8slbrp instance");
}

std::string K8slbrp::buildK8sLbrpCode(std::string const &k8slbrp_code, K8slbrpPortModeEnum port_mode) {
    if (port_mode == K8slbrpPortModeEnum::SINGLE) {
        return "#define SINGLE_PORT_MODE 1\n" + k8slbrp_code;
    }
    return "#define SINGLE_PORT_MODE 0\n" + k8slbrp_code;
}

void K8slbrp::flood_packet(Ports &port, PacketInMetadata &md,
                           const std::vector<uint8_t> &packet) {
    EthernetII p(&packet[0], packet.size());

    for (auto &it: get_ports()) {
        if (it->name() == port.name()) {
            continue;
        }
        it->send_packet_out(p);
        logger()->trace("sent pkt to port {0} as result of flooding", it->name());
    }
}

void K8slbrp::packet_in(Ports &port,
                        polycube::service::PacketInMetadata &md,
                        const std::vector<uint8_t> &packet) {
    try {
        switch (static_cast<SlowPathReason>(md.reason)) {
            case SlowPathReason::ARP_REPLY: {
                logger()->debug("received pkt in slowpath - reason: ARP_REPLY");
                EthernetII eth(&packet[0], packet.size());

                auto b_port = getBackendPort();
                if (!b_port)
                    return;

                // send the original packet
                logger()->debug("sending original ARP reply to backend port: {}",
                                b_port->name());
                b_port->send_packet_out(eth);

                // send a second packet for the virtual src IPs
                ARP &arp = eth.rfind_pdu<ARP>();

                uint32_t ip = uint32_t(arp.sender_ip_addr());
                ip &= htonl(src_ip_rewrite_->mask);
                ip |= htonl(src_ip_rewrite_->net);

                IPv4Address addr(ip);
                arp.sender_ip_addr(addr);

                logger()->debug("sending modified ARP reply to backend port: {}",
                                b_port->name());
                b_port->send_packet_out(eth);
                break;
            }
            case SlowPathReason::FLOODING: {
                logger()->debug("received pkt in slowpath - reason: FLOODING");
                flood_packet(port, md, packet);
                break;
            }
            default: {
                logger()->error("not valid reason {0} received", md.reason);
            }
        }
    } catch (const std::exception &e) {
        logger()->error("exception during slowpath packet processing: '{0}'", e.what());
    }
}

void K8slbrp::reloadCodeWithNewPorts() {
    uint16_t frontend_port = 0;
    uint16_t backend_port = 1;

    for (auto &it: get_ports()) {
        if (it->getType() == PortsTypeEnum::FRONTEND) {
            frontend_port = it->index();
        } else backend_port = it->index();
    }

    logger()->debug("reloading code with FRONTEND port {0} and BACKEND port {1}",
                    frontend_port, backend_port);
    std::string frontend_port_str("#define FRONTEND_PORT " + std::to_string(frontend_port));
    std::string backend_port_str("#define BACKEND_PORT " + std::to_string(backend_port));

    reload(frontend_port_str + "\n" + backend_port_str + "\n" + k8slbrp_code_);

    logger()->trace("new k8slbrp code loaded");
}

void K8slbrp::reloadCodeWithNewBackendPort(uint16_t backend_port_index) {
    logger()->debug("reloading code with BACKEND port {0}", backend_port_index);
    std::string backend_port_str("#define BACKEND_PORT " + std::to_string(backend_port_index));

    reload(backend_port_str + "\n" + k8slbrp_code_);

    logger()->trace("new k8slbrp code loaded");
}

std::vector<std::shared_ptr<Ports>> K8slbrp::getFrontendPorts() {
    std::vector<std::shared_ptr<Ports>> frontendPorts;
    for (auto &it: get_ports()) {
        if (it->getType() == PortsTypeEnum::FRONTEND) {
            frontendPorts.push_back(it);
        }
    }
    return frontendPorts;
}

std::shared_ptr<Ports> K8slbrp::getBackendPort() {
    for (auto &it: get_ports()) {
        if (it->getType() == PortsTypeEnum::BACKEND) {
            return it;
        }
    }
    return nullptr;
}

std::shared_ptr<Ports> K8slbrp::getPorts(const std::string &name) {
    return get_port(name);
}

std::vector<std::shared_ptr<Ports>> K8slbrp::getPortsList() {
    return get_ports();
}

void K8slbrp::addPortsInSinglePortMode(const std::string &name, const PortsJsonObject &conf) {
    PortsTypeEnum type = conf.getType();
    auto ipIsSet = conf.ipIsSet();
    if (get_ports().size() == 2) throw std::runtime_error("reached maximum number of ports");

    if (type == PortsTypeEnum::FRONTEND) {
        if (getFrontendPorts().size() == 1) {
            throw std::runtime_error(
                    "there is already a FRONTEND port. Only a FRONTEND port is allowed in SINGLE port mode"
            );
        }
        if (ipIsSet) {
            throw std::runtime_error("the ip address in not allow in SINGLE port mode for a FRONTEND port");
        }
    } else {
        if (getBackendPort() != nullptr) throw std::runtime_error("there is already a BACKEND port");
        if (ipIsSet) throw std::runtime_error("the ip address in not allowed in BACKEND port");
    }
    add_port<PortsJsonObject>(name, conf);
    if (get_ports().size() == 2) reloadCodeWithNewPorts();
}

void K8slbrp::addPortsInMultiPortMode(const std::string &name, const PortsJsonObject &conf) {
    PortsTypeEnum type = conf.getType();
    auto ipIsSet = conf.ipIsSet();

    if (type == PortsTypeEnum::FRONTEND) {
        if (!ipIsSet) throw std::runtime_error("the IP address is mandatory in MULTI port mode for a FRONTEND port");
        auto ip = conf.getIp();
        auto found = frontend_ip_set_.find(ip) != frontend_ip_set_.end();
        if (found) {
            throw std::runtime_error("a FRONTEND port with the provided IP address (" + ip + ") already exist");
        }
        if (!frontend_ip_set_.insert(ip).second) {
            throw std::runtime_error("failed to set the IP address for the new FRONTEND port");
        }
        try {
            auto created_port = add_port<PortsJsonObject>(name, conf);
            auto ip_to_frontend_port_table = get_hash_table<uint32_t, uint16_t>(EBPF_IP_TO_FRONTEND_PORT_MAP);
            ip_to_frontend_port_table.set(
                    utils::ip_string_to_nbo_uint(created_port->getIp()),
                    created_port->index()
            );
        } catch (std::exception &ex) {
            frontend_ip_set_.erase(ip);
            throw;
        }
    } else {
        if (getBackendPort() != nullptr) throw std::runtime_error("there is already a BACKEND port");
        if (ipIsSet) throw std::runtime_error("the ip address in not allowed in BACKEND port");
        auto created_port = add_port<PortsJsonObject>(name, conf);
        reloadCodeWithNewBackendPort(created_port->index());
    }
}

void K8slbrp::addPorts(const std::string &name, const PortsJsonObject &conf) {
    try {
        if (port_mode_ == K8slbrpPortModeEnum::SINGLE) addPortsInSinglePortMode(name, conf);
        else addPortsInMultiPortMode(name, conf);
    } catch (std::runtime_error &ex) {
        logger()->warn("failed to add port {0}: {1}", name, ex.what());
        throw;
    }

    logger()->info("created new Port with name {0}", name);
}

void K8slbrp::addPortsList(const std::vector<PortsJsonObject> &conf) {
    for (auto &i: conf) {
        std::string name_ = i.getName();
        addPorts(name_, i);
    }
}

void K8slbrp::replacePorts(const std::string &name, const PortsJsonObject &conf) {
    delPorts(name);
    std::string name_ = conf.getName();
    addPorts(name_, conf);
}

void K8slbrp::delPorts(const std::string &name) {
    logger()->info("received request for K8slbrp port deletion");
    try {
        auto port = get_port(name);
        if (this->port_mode_ == K8slbrpPortModeEnum::MULTI && port->getType() == PortsTypeEnum::FRONTEND) {
            auto ip = port->getIp();

            logger()->trace("retrieving ip-to-FrontendPort kernel map");
            auto ip_to_frontend_port_table = get_hash_table<uint32_t, uint16_t>(EBPF_IP_TO_FRONTEND_PORT_MAP);
            logger()->trace("retrieved ip-to-FrontendPort kernel map");
            logger()->trace("trying to remove the port IP address from ip-to-FrontendPort kernel map");
            ip_to_frontend_port_table.remove(utils::ip_string_to_nbo_uint(ip));
            logger()->trace("removed the port IP address from ip-to-FrontendPort kernel map");

            if (this->frontend_ip_set_.erase(ip) == 0) {
                throw std::runtime_error("failed to delete the port IP address for the new FRONTEND port");
            }
        }
        logger()->trace("trying to remove the port");
        remove_port(name);
    } catch (std::exception &ex) {
        logger()->warn("failed to delete port {0}: {1}", name, ex.what());
        throw;
    }
    logger()->info("deleted K8slbrp port");
}

void K8slbrp::delPortsList() {
    auto ports = get_ports();
    for (auto it: ports) {
        delPorts(it->name());
    }
}

K8slbrpPortModeEnum K8slbrp::getPortMode() {
    return port_mode_;
}

void K8slbrp::setPortMode(const K8slbrpPortModeEnum &value) {
    throw std::runtime_error("port mode cannot be changed at runtime");
}

std::shared_ptr<SrcIpRewrite> K8slbrp::getSrcIpRewrite() {
    return src_ip_rewrite_;
}

void K8slbrp::addSrcIpRewrite(const SrcIpRewriteJsonObject &value) {
    logger()->debug(
            "[SrcIpRewrite] received request to create SrcIpRewrite range {0} , new "
            "ip range {1}",
            value.getIpRange(), value.getNewIpRange());

    src_ip_rewrite_ = std::make_shared<SrcIpRewrite>(*this, value);
    src_ip_rewrite_->update(value);
}

void K8slbrp::replaceSrcIpRewrite(const SrcIpRewriteJsonObject &conf) {
    delSrcIpRewrite();
    addSrcIpRewrite(conf);
}

void K8slbrp::delSrcIpRewrite() {
    // what the hell means to remove entry in this case?
}

std::shared_ptr<Service>
K8slbrp::getService(const std::string &vip, const uint16_t &vport, const ServiceProtoEnum &proto) {
    // This method retrieves the pointer to Service object specified by its keys.
    logger()->debug("[Service] received request to read a service entry");
    logger()->debug("[Service] virtual IP: {0}, virtual port: {1}, protocol: {2}",
                    vip, vport,
                    ServiceJsonObject::ServiceProtoEnum_to_string(proto));

    uint16_t vp = vport;
    if (Service::convertProtoToNumber(proto) == 1)
        vp = vport;

    Service::ServiceKey key =
            Service::ServiceKey(vip, vp, Service::convertProtoToNumber(proto));

    if (service_map_.count(key) == 0) {
        logger()->error("[Service] there are no entries associated with that key");
        throw std::runtime_error("there are no entries associated with that key");
    }

    return std::shared_ptr<Service>(&service_map_.at(key), [](Service *) {});
}

std::vector<std::shared_ptr<Service>> K8slbrp::getServiceList() {
    std::vector<std::shared_ptr<Service>> services_vect;

    for (auto &it: service_map_) {
        Service::ServiceKey key = it.first;
        services_vect.push_back(
                getService(std::get<0>(key), std::get<1>(key),
                           Service::convertNumberToProto(std::get<2>(key))));
    }

    return services_vect;
}

void K8slbrp::addService(const std::string &vip, const uint16_t &vport, const ServiceProtoEnum &proto,
                         const ServiceJsonObject &conf) {
    logger()->debug("[Service] received request to create new service entry");
    logger()->debug(
            "[Service] virtual IP: {0}, virtual port: {1}, protocol: {2}", vip, vport,
            ServiceJsonObject::ServiceProtoEnum_to_string(proto)
    );

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
    } else if (proto == ServiceProtoEnum::ICMP && conf.getVport() != Service::ICMP_EBPF_PORT) {
        throw std::runtime_error(
                "ICMP Service requires 0 as virtual port since this parameter is useless for ICMP services"
        );
    }

    Service::ServiceKey key =
            Service::ServiceKey(vip, vport, Service::convertProtoToNumber(proto));
    if (service_map_.count(key) != 0) {
        logger()->error("[Service] This service already exists");
        throw std::runtime_error("This service already exists");
    }
    Service service = Service(*this, conf);
    service_map_.insert(std::make_pair(key, service));
}

void K8slbrp::addServiceList(const std::vector<ServiceJsonObject> &conf) {
    for (auto &i: conf) {
        std::string vip_ = i.getVip();
        uint16_t vport_ = i.getVport();
        ServiceProtoEnum proto_ = i.getProto();
        addService(vip_, vport_, proto_, i);
    }
}

void K8slbrp::replaceService(const std::string &vip, const uint16_t &vport, const ServiceProtoEnum &proto,
                             const ServiceJsonObject &conf) {
    delService(vip, vport, proto);
    std::string vip_ = conf.getVip();
    uint16_t vport_ = conf.getVport();
    ServiceProtoEnum proto_ = conf.getProto();
    addService(vip_, vport_, proto_, conf);
}

void K8slbrp::delService(const std::string &vip, const uint16_t &vport, const ServiceProtoEnum &proto) {
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

void K8slbrp::delServiceList() {
    for (auto it = service_map_.begin(); it != service_map_.end();) {
        auto tmp = it;
        it++;
        delService(tmp->second.getVip(), tmp->second.getVport(),
                   tmp->second.getProto());
    }
}


