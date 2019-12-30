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

#include "cube.h"

#include <net/if.h>
#include "port_tc.h"
#include "port_xdp.h"
#include "utils/ns.h"
#include "polycube/services/utils.h"

const std::string prefix_ns = "pcn-";
const std::string prefix_port = "ns_port_";

namespace polycube {
    namespace polycubed {

        Cube::Cube(const std::string &name, const std::string &service_name,
                   PatchPanel &patch_panel_ingress, PatchPanel &patch_panel_egress,
                   LogLevel level, CubeType type, bool shadow, bool span)
                : BaseCube(name, service_name, MASTER_CODE, patch_panel_ingress,
                           patch_panel_egress, level, type), shadow_(shadow), span_(span) {
            std::lock_guard <std::mutex> guard(bcc_mutex);

            auto forward_ = master_program_->get_array_table<uint32_t>("forward_chain_");
            forward_chain_ = std::unique_ptr < ebpf::BPFArrayTable < uint32_t >> (
                    new ebpf::BPFArrayTable<uint32_t>(forward_));

            // add free ports
            for (uint16_t i = 0; i < _POLYCUBE_MAX_PORTS; i++)
                free_ports_.insert(i);

            if (!shadow_ && span_) {
                throw std::runtime_error("Span mode is not present in no-shadow services");
            }
            if (shadow_) {
                std::string name_ns = prefix_ns + name;
                Namespace ns = Namespace::create(name_ns);
                ns.set_random_id();
            }
        }

        Cube::~Cube() {
            for (auto &it : ports_by_name_) {
                it.second->set_peer("");
            }
            if (get_shadow()) {
                std::string name_ns = prefix_ns + get_name();
                Namespace ns = Namespace::open(name_ns);
                ns.remove();
            }
        }

        std::string Cube::get_wrapper_code() {
            return BaseCube::get_wrapper_code() + CUBE_WRAPPER;
        }

        void Cube::set_conf(const nlohmann::json &conf) {
            // Ports are handled in the service implementation directly
            return BaseCube::set_conf(conf);
        }

        nlohmann::json Cube::to_json() const {
            nlohmann::json j;
            j.update(BaseCube::to_json());

            json ports_json = json::array();

            for (auto &it : ports_by_name_) {
                json port_json = json::object();
                port_json["name"] = it.second->name();
                auto peer = it.second->peer();

                if (peer.size() > 0) {
                    port_json["peer"] = peer;
                }

                ports_json += port_json;
            }

            if (ports_json.size() > 0) {
                j["ports"] = ports_json;
            }

            j["shadow"] = shadow_;
            j["span"] = span_;

            return j;
        }

        uint16_t Cube::allocate_port_id() {
            if (free_ports_.size() < 1) {
                logger->error("there are not free ports in cube '{0}'", name_);
                throw std::runtime_error("No free ports");
            }
            uint16_t p = *free_ports_.begin();
            free_ports_.erase(p);
            return p;
        }

        void Cube::release_port_id(uint16_t id) {
            free_ports_.insert(id);
        }

        std::shared_ptr <PortIface> Cube::add_port(const std::string &name,
                                                   const nlohmann::json &conf) {
            std::lock_guard <std::mutex> cube_guard(cube_mutex_);
            if (ports_by_name_.count(name) != 0) {
                throw std::runtime_error("Port " + name + " already exists");
            }
            if (get_shadow() && (name.find(prefix_port) != std::string::npos)) {
                throw std::runtime_error("Port name cannot contain '" + prefix_port + "'");
            }
            auto id = allocate_port_id();

            std::shared_ptr <PortIface> port;

            switch (type_) {
                case CubeType::TC:
                    port = std::make_shared<PortTC>(*this, name, id, conf);
                    break;
                case CubeType::XDP_SKB:
                case CubeType::XDP_DRV:
                    port = std::make_shared<PortXDP>(*this, name, id, conf);
                    break;
            }

            ports_by_name_.emplace(name, port);
            ports_by_index_.emplace(id, port);

            // TODO: is this valid?
            cube_mutex_.unlock();
            try {
                if (conf.count("peer")) {
                    port->set_peer(conf.at("peer").get<std::string>());
                }
            } catch (...) {
                ports_by_name_.erase(name);
                ports_by_index_.erase(id);
                throw;
            }

            if (get_shadow()) {
                // create a veth and move it in the namespace
                std::string name_peerB = get_name() + "-" + name;
                std::shared_ptr <Veth> veth = std::make_shared<Veth>(Veth::create(name, name_peerB));
                VethPeer peerA = veth->get_peerA();
                VethPeer peerB = veth->get_peerB();
                int ifindex = if_nametoindex(name.c_str());

                peerB.set_status(IFACE_STATUS::UP);
                peerA.set_namespace(prefix_ns + get_name());
                peerA.set_status(IFACE_STATUS::UP);
                if (conf.count("ip")) {
                    std::string ip_address;
                    std::string netmask;
                    polycube::service::utils::split_ip_and_prefix(
                            conf.at("ip").get<std::string>(), ip_address, netmask);
                    int prefix = polycube::service::utils::get_netmask_length(netmask);
                    peerA.set_ip(ip_address, prefix);
                } else if (conf.count("ipv6")) {
                    peerA.set_ipv6(conf.at("ipv6").get<std::string>());
                }
                if (conf.count("mac")) {
                    peerA.set_mac(conf.at("mac").get<std::string>());
                }

                std::string name2 = prefix_port + name;
                nlohmann::json conf2 = nlohmann::json::object();
                conf2["name"] = name2;
                conf2["peer"] = name_peerB;

                auto id2 = allocate_port_id();
                std::shared_ptr <PortIface> port2;

                switch (type_) {
                    case CubeType::TC:
                        port2 = std::make_shared<PortTC>(*this, name2, id2, conf2);
                        break;
                    case CubeType::XDP_SKB:
                    case CubeType::XDP_DRV:
                        port2 = std::make_shared<PortXDP>(*this, name2, id2, conf2);
                        break;
                }

                ports_by_name_.emplace(name2, port2);
                ports_by_index_.emplace(id2, port2);
                veth_by_name_.emplace(name, veth);
                ifindex_veth_.emplace(ifindex, name);

                try {
                    port2->set_peer(name_peerB);
                } catch (...) {
                    ports_by_name_.erase(name2);
                    ports_by_index_.erase(id2);
                    veth_by_name_.erase(name);
                    ifindex_veth_.erase(ifindex);
                    throw;
                }
            }

            return std::move(port);
        }

        void Cube::remove_port(const std::string &name) {
            if (ports_by_name_.count(name) == 0) {
                throw std::runtime_error("Port " + name + " does not exist");
            }

            // set_peer("") has to be done before acquiring cube_guard because
            // a dead lock is possible when destroying the port:
            // lock(cube_mutex_) -> ~Port::Port() -> Port::set_peer() ->
            // ServiceController::set_{tc,xdp}_port_peer() ->
            // Port::unconnect() -> Cube::update_forwarding_table -> lock(cube_mutex_)

            ports_by_name_.at(name)->set_peer("");

            std::lock_guard <std::mutex> cube_guard(cube_mutex_);
            auto index = ports_by_name_.at(name)->index();
            ports_by_name_.erase(name);
            ports_by_index_.erase(index);
            release_port_id(index);

            cube_mutex_.unlock();
            if (get_shadow()) {
                // remove the veth
                try {
                    veth_by_name_.at(name)->remove();
                    veth_by_name_.erase(name);
                } catch (...) {
                    logger->trace("veth {0} not found", name);
                }

                for (auto const&[key, val] : ifindex_veth_) {
                    if (val == name) {
                        ifindex_veth_.erase(key);
                        break;
                    }
                }

                // remove the second port
                std::string name2 = prefix_port + name;
                ports_by_name_.at(name2)->set_peer("");
                auto index2 = ports_by_name_.at(name2)->index();
                ports_by_name_.erase(name2);
                ports_by_index_.erase(index2);
                release_port_id(index2);
            }
        }

        std::shared_ptr <PortIface> Cube::get_port(const std::string &name) {
            std::lock_guard <std::mutex> cube_guard(cube_mutex_);
            if (ports_by_name_.count(name) == 0) {
                throw std::runtime_error("Port " + name + " does not exist");
            }
            return ports_by_name_.at(name);
        }

        std::map <std::string, std::shared_ptr<PortIface>> &Cube::get_ports() {
            return ports_by_name_;
        }

        void Cube::update_forwarding_table(int index, int value) {
            std::lock_guard <std::mutex> cube_guard(cube_mutex_);
            if (forward_chain_)  // is the forward chain still active?
                forward_chain_->update_value(index, value);
        }

        const bool Cube::get_shadow() const {
            return shadow_;
        }

        const bool Cube::get_span() const {
            return span_;
        }

        void Cube::set_span(const bool value) {
            if (shadow_)
                span_ = value;
            else
                throw std::runtime_error("Span mode is not present in no-shadow services");
        }

/* returns the name of the veth given the ifindex */
        const std::string Cube::get_veth_name_from_index(const int ifindex) {
            std::string name;
            try {
                name = ifindex_veth_.at(ifindex);
            } catch (...) {
                name = "";
            }
            return name;
        }

        const std::string Cube::MASTER_CODE = R"(
// table used to save ports to endpoint relation
BPF_TABLE_SHARED("array", int, u32, forward_chain_, _POLYCUBE_MAX_PORTS);
)";

        const std::string Cube::CUBE_WRAPPER = R"(
BPF_TABLE("extern", int, u32, forward_chain_, _POLYCUBE_MAX_PORTS);
)";

    }  // namespace polycubed
}  // namespace polycube
