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

#include "api/BridgeApi.h"
#include "polycube/services/cube.h"
#include "polycube/services/port.h"
#include "polycube/services/utils.h"

#include "BridgeSTP.h"
#include "Bridge_dp_stp.h"
#include "Bridge_dp_no_stp.h"

#include <tins/tins.h>
#include <tins/ethernetII.h>
#include <tins/dot1q.h>

using namespace io::swagger::server::model;

enum class SlowPathReason { BPDU = 1, BROADCAST };
enum class PortMode { ACCESS, TRUNK };

const std::string mode_to_string(PortMode mode);
PortMode string_to_mode(const std::string &str);

#define VLAN_WILDCARD 0x00

class Bridge;

/* definitions copied from datapath */
struct port_vlan_key {
  // todo use u16 instead of u32
  uint32_t port;
  uint32_t vlan;
} __attribute__((packed));

struct port_vlan_value {
  uint16_t vlan;
  uint16_t stp_state;
} __attribute__((packed));

struct port {
  uint16_t mode;
  uint16_t native_vlan;
} __attribute__((packed));

struct fwd_key {
  uint32_t vlan;
  uint64_t mac;
} __attribute__((packed));

struct fwd_entry {
  uint64_t timestamp;
  uint32_t port;
} __attribute__((packed));

class BridgePort : public polycube::service::Port {
 public:
  //BridgePort(Bridge &parent, Port &p);
  BridgePort(polycube::service::Cube<BridgePort> &parent,
             std::shared_ptr<polycube::service::PortIface> port,
             const PortsSchema &conf);
  ~BridgePort();
  PortMode mode() const;
  void set_mode(PortMode mode);
  uint16_t access_vlan() const;
  void set_access_vlan(uint16_t vlan);
  void add_trunk_vlan(uint16_t vlan);
  void remove_trunk_vlan(uint16_t vlan);
  uint16_t native_vlan() const;
  void set_native_vlan(uint16_t vlan);
  bool is_trunk_vlan_allowed(uint16_t vlan);
  void cleanup();

  /*
   * sends a packet through the port, the packet is automatically tagged or
   * untagged if required. Packet is dropped is port status does not allow to
   * send packets of if the vlan is not allowed.
   */
  void send_packet_out(const std::vector<uint8_t> &packet, bool tagged, uint16_t vlan);

  BridgePort & operator=(const BridgePort&) = delete;
  BridgePort(const BridgePort&) = delete;

 private:
  Bridge &parent;
  void cleanup_trunk_port();
  void cleanup_access_port();

  PortMode mode_;
  uint16_t access_vlan_;
  uint16_t native_vlan_;
  std::set<uint16_t> trunk_vlans_;
  std::shared_ptr<spdlog::logger> l;
};

class Bridge : public polycube::service::Cube<BridgePort> {
friend class BridgeSTP;
friend class BridgePort;
 public:
  Bridge(const std::string &name, BridgeSchema &conf, CubeType type = CubeType::TC, const std::string &code = bridge_code);
  ~Bridge();

  PortsSchema add_port(const std::string &port_name, const PortsSchema &port);
  void remove_port(const std::string &port_name);

  void packet_in(BridgePort &port, PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

   // TODO: comment about these functions
  FilteringdatabaseSchema create_bridge_filteringdatabase_by_id(const std::string &vlan, const std::string &address, const FilteringdatabaseSchema &filteringdatabase);
  std::vector<FilteringdatabaseSchema> create_bridge_filteringdatabase_list_by_id(const std::vector<FilteringdatabaseSchema> &filteringdatabase);
  PortsAccessSchema create_bridge_ports_access_by_id(const std::string &portsName, const PortsAccessSchema &access);
  PortsStpSchema create_bridge_ports_stp_by_id(const std::string &portsName, const std::string &vlan, const PortsStpSchema &stp);
  std::vector<PortsStpSchema> create_bridge_ports_stp_list_by_id(const std::string &portsName, const std::vector<PortsStpSchema> &stp);
  PortsTrunkAllowedSchema create_bridge_ports_trunk_allowed_by_id(const std::string &portsName, const std::string &vlanid, const PortsTrunkAllowedSchema &allowed);
  std::vector<PortsTrunkAllowedSchema> create_bridge_ports_trunk_allowed_list_by_id(const std::string &portsName, const std::vector<PortsTrunkAllowedSchema> &allowed);
  PortsTrunkSchema create_bridge_ports_trunk_by_id(const std::string &portsName, const PortsTrunkSchema &trunk);
  StpSchema create_bridge_stp_by_id(const std::string &vlan, const StpSchema &stp);
  std::vector<StpSchema> create_bridge_stp_list_by_id(const std::vector<StpSchema> &stp);
  void delete_bridge_filteringdatabase_by_id(const std::string &vlan, const std::string &address);
  void delete_bridge_filteringdatabase_list_by_id();
  void delete_bridge_ports_access_by_id(const std::string &portsName);
  void delete_bridge_ports_stp_by_id(const std::string &portsName, const std::string &vlan);
  void delete_bridge_ports_stp_list_by_id(const std::string &portsName);
  void delete_bridge_ports_trunk_allowed_by_id(const std::string &portsName, const std::string &vlanid);
  void delete_bridge_ports_trunk_allowed_list_by_id(const std::string &portsName);
  void delete_bridge_ports_trunk_by_id(const std::string &portsName);
  void delete_bridge_stp_by_id(const std::string &vlan);
  void delete_bridge_stp_list_by_id();
  int32_t read_bridge_agingtime_by_id();
  BridgeSchema read_bridge_by_id();
  int32_t read_bridge_filteringdatabase_age_by_id(const std::string &vlan, const std::string &address);
  FilteringdatabaseSchema read_bridge_filteringdatabase_by_id(const std::string &vlan, const std::string &address);
  std::string read_bridge_filteringdatabase_entrytype_by_id(const std::string &vlan, const std::string &address);
  std::vector<FilteringdatabaseSchema> read_bridge_filteringdatabase_list_by_id();
  std::string read_bridge_filteringdatabase_port_by_id(const std::string &vlan, const std::string &address);
  PortsAccessSchema read_bridge_ports_access_by_id(const std::string &portsName);
  int32_t read_bridge_ports_access_vlanid_by_id(const std::string &portsName);
  std::string read_bridge_ports_address_by_id(const std::string &portsName);
  PortsSchema read_bridge_ports_by_id(const std::string &portsName);
  std::string read_bridge_ports_mode_by_id(const std::string &portsName);
  PortsStpSchema read_bridge_ports_stp_by_id(const std::string &portsName, const std::string &vlan);
  std::vector<PortsStpSchema> read_bridge_ports_stp_list_by_id(const std::string &portsName);
  int32_t read_bridge_ports_stp_pathcost_by_id(const std::string &portsName, const std::string &vlan);
  int32_t read_bridge_ports_stp_portpriority_by_id(const std::string &portsName, const std::string &vlan);
  std::string read_bridge_ports_stp_state_by_id(const std::string &portsName, const std::string &vlan);
  PortsTrunkAllowedSchema read_bridge_ports_trunk_allowed_by_id(const std::string &portsName, const std::string &vlanid);
  std::vector<PortsTrunkAllowedSchema> read_bridge_ports_trunk_allowed_list_by_id(const std::string &portsName);
  PortsTrunkSchema read_bridge_ports_trunk_by_id(const std::string &portsName);
  int32_t read_bridge_ports_trunk_nativevlan_by_id(const std::string &portsName);
  std::string read_bridge_stp_address_by_id(const std::string &vlan);
  StpSchema read_bridge_stp_by_id(const std::string &vlan);
  int32_t read_bridge_stp_forwarddelay_by_id(const std::string &vlan);
  int32_t read_bridge_stp_hellotime_by_id(const std::string &vlan);
  std::vector<StpSchema> read_bridge_stp_list_by_id();
  int32_t read_bridge_stp_maxmessageage_by_id(const std::string &vlan);
  int32_t read_bridge_stp_priority_by_id(const std::string &vlan);
  bool read_bridge_stpenabled_by_id();
  std::string read_bridge_type_by_id();
  void update_bridge_agingtime_by_id(const int32_t &agingtime);
  void update_bridge_by_id(const BridgeSchema &bridge);
  void update_bridge_filteringdatabase_by_id(const std::string &vlan, const std::string &address, const FilteringdatabaseSchema &filteringdatabase);
  void update_bridge_filteringdatabase_entrytype_by_id(const std::string &vlan, const std::string &address, const std::string &entrytype);
  void update_bridge_filteringdatabase_list_by_id(const std::vector<FilteringdatabaseSchema> &filteringdatabase);
  void update_bridge_filteringdatabase_port_by_id(const std::string &vlan, const std::string &address, const std::string &port);
  void update_bridge_ports_access_by_id(const std::string &portsName, const PortsAccessSchema &access);
  void update_bridge_ports_access_vlanid_by_id(const std::string &portsName, const int32_t &vlanid);
  void update_bridge_ports_address_by_id(const std::string &portsName, const std::string &address);
  void update_bridge_ports_mode_by_id(const std::string &portsName, const std::string &mode);
  void update_bridge_ports_stp_by_id(const std::string &portsName, const std::string &vlan, const PortsStpSchema &stp);
  void update_bridge_ports_stp_list_by_id(const std::string &portsName, const std::vector<PortsStpSchema> &stp);
  void update_bridge_ports_stp_pathcost_by_id(const std::string &portsName, const std::string &vlan, const int32_t &pathcost);
  void update_bridge_ports_stp_portpriority_by_id(const std::string &portsName, const std::string &vlan, const int32_t &portpriority);
  void update_bridge_ports_trunk_allowed_by_id(const std::string &portsName, const std::string &vlanid, const PortsTrunkAllowedSchema &allowed);
  void update_bridge_ports_trunk_allowed_list_by_id(const std::string &portsName, const std::vector<PortsTrunkAllowedSchema> &allowed);
  void update_bridge_ports_trunk_by_id(const std::string &portsName, const PortsTrunkSchema &trunk);
  void update_bridge_ports_trunk_nativevlan_by_id(const std::string &portsName, const int32_t &nativevlan);
  void update_bridge_stp_address_by_id(const std::string &vlan, const std::string &address);
  void update_bridge_stp_by_id(const std::string &vlan, const StpSchema &stp);
  void update_bridge_stp_forwarddelay_by_id(const std::string &vlan, const int32_t &forwarddelay);
  void update_bridge_stp_hellotime_by_id(const std::string &vlan, const int32_t &hellotime);
  void update_bridge_stp_list_by_id(const std::vector<StpSchema> &stp);
  void update_bridge_stp_maxmessageage_by_id(const std::string &vlan, const int32_t &maxmessageage);
  void update_bridge_stp_priority_by_id(const std::string &vlan, const int32_t &priority);
  void update_bridge_stpenabled_by_id(const bool &stpenabled);
  void update_bridge_type_by_id(const std::string &type);

  bool isStpEnabled();

private:
  std::string generate_code(bool stp_enabled);
  // internal control API

  void broadcast_packet(Port &port, PacketInMetadata &md, const std::vector<uint8_t> &packet);
  void process_bpdu(Port &port, PacketInMetadata &md, const std::vector<uint8_t> &packet);
  BridgePort &get_bridge_port(const std::string &name);
  BridgePort &get_bridge_port(int port_id);
  BridgeSTP &get_stp_instance(uint16_t vlan_id, bool create = false);

  int module_index;
  std::unordered_map<uint16_t, BridgeSTP> stps_;
  uint16_t aging_time_;
  bool stp_enabled_;

  std::mutex ports_mutex_;
};
