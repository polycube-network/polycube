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

#include "Bridge.h"
#include <time.h>

using namespace io::swagger::server::model;

/*
 * These functions do not include a default implementation and should be
 * implemented by the user
 */

/**
* @brief   Create filteringdatabase by ID
*
* Create operation of resource: filteringdatabase*
*
* @param[in] vlan ID of vlan
* @param[in] address ID of address
* @param[in] filteringdatabase filteringdatabasebody object
*
* Responses:
* FilteringdatabaseSchema
*/
FilteringdatabaseSchema
Bridge::create_bridge_filteringdatabase_by_id(const std::string &vlan, const std::string &address, const FilteringdatabaseSchema &filteringdatabase) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Create filteringdatabase by ID
*
* Create operation of resource: filteringdatabase*
*
* @param[in] filteringdatabase filteringdatabasebody object
*
* Responses:
* std::vector<FilteringdatabaseSchema>
*/
std::vector<FilteringdatabaseSchema>
Bridge::create_bridge_filteringdatabase_list_by_id(const std::vector<FilteringdatabaseSchema> &filteringdatabase) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Create access by ID
*
* Create operation of resource: access*
*
* @param[in] portsName ID of ports_name
* @param[in] access accessbody object
*
* Responses:
* PortsAccessSchema
*/
PortsAccessSchema
Bridge::create_bridge_ports_access_by_id(const std::string &portsName, const PortsAccessSchema &access) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Create stp by ID
*
* Create operation of resource: stp*
*
* @param[in] portsName ID of ports_name
* @param[in] vlan ID of vlan
* @param[in] stp stpbody object
*
* Responses:
* PortsStpSchema
*/
PortsStpSchema
Bridge::create_bridge_ports_stp_by_id(const std::string &portsName, const std::string &vlan, const PortsStpSchema &stp) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Create stp by ID
*
* Create operation of resource: stp*
*
* @param[in] portsName ID of ports_name
* @param[in] stp stpbody object
*
* Responses:
* std::vector<PortsStpSchema>
*/
std::vector<PortsStpSchema>
Bridge::create_bridge_ports_stp_list_by_id(const std::string &portsName, const std::vector<PortsStpSchema> &stp) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Create allowed by ID
*
* Create operation of resource: allowed*
*
* @param[in] portsName ID of ports_name
* @param[in] vlanid ID of vlanid
* @param[in] allowed allowedbody object
*
* Responses:
* PortsTrunkAllowedSchema
*/
PortsTrunkAllowedSchema
Bridge::create_bridge_ports_trunk_allowed_by_id(const std::string &portsName, const std::string &vlanid, const PortsTrunkAllowedSchema &allowed) {
  auto &&p = get_bridge_port(portsName);
  p.add_trunk_vlan(std::stoul(vlanid));
  PortsTrunkAllowedSchema ports_trunk_allowed;
  ports_trunk_allowed.setVlanid(std::stoul(vlanid));
  //TODO return fulfilled schema
  return ports_trunk_allowed;
}

/**
* @brief   Create allowed by ID
*
* Create operation of resource: allowed*
*
* @param[in] portsName ID of ports_name
* @param[in] allowed allowedbody object
*
* Responses:
* std::vector<PortsTrunkAllowedSchema>
*/
std::vector<PortsTrunkAllowedSchema>
Bridge::create_bridge_ports_trunk_allowed_list_by_id(const std::string &portsName, const std::vector<PortsTrunkAllowedSchema> &allowed) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Create trunk by ID
*
* Create operation of resource: trunk*
*
* @param[in] portsName ID of ports_name
* @param[in] trunk trunkbody object
*
* Responses:
* PortsTrunkSchema
*/
PortsTrunkSchema
Bridge::create_bridge_ports_trunk_by_id(const std::string &portsName, const PortsTrunkSchema &trunk) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Create stp by ID
*
* Create operation of resource: stp*
*
* @param[in] vlan ID of vlan
* @param[in] stp stpbody object
*
* Responses:
* StpSchema
*/
StpSchema
Bridge::create_bridge_stp_by_id(const std::string &vlan, const StpSchema &stp) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Create stp by ID
*
* Create operation of resource: stp*
*
* @param[in] stp stpbody object
*
* Responses:
* std::vector<StpSchema>
*/
std::vector<StpSchema>
Bridge::create_bridge_stp_list_by_id(const std::vector<StpSchema> &stp) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Delete filteringdatabase by ID
*
* Delete operation of resource: filteringdatabase*
*
* @param[in] vlan ID of vlan
* @param[in] address ID of address
*
* Responses:
*
*/
void
Bridge::delete_bridge_filteringdatabase_by_id(const std::string &vlan, const std::string &address) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Delete filteringdatabase by ID
*
* Delete operation of resource: filteringdatabase*
*
*
* Responses:
*
*/
void
Bridge::delete_bridge_filteringdatabase_list_by_id() {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Delete access by ID
*
* Delete operation of resource: access*
*
* @param[in] portsName ID of ports_name
*
* Responses:
*
*/
void
Bridge::delete_bridge_ports_access_by_id(const std::string &portsName) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Delete stp by ID
*
* Delete operation of resource: stp*
*
* @param[in] portsName ID of ports_name
* @param[in] vlan ID of vlan
*
* Responses:
*
*/
void
Bridge::delete_bridge_ports_stp_by_id(const std::string &portsName, const std::string &vlan) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Delete stp by ID
*
* Delete operation of resource: stp*
*
* @param[in] portsName ID of ports_name
*
* Responses:
*
*/
void
Bridge::delete_bridge_ports_stp_list_by_id(const std::string &portsName) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Delete allowed by ID
*
* Delete operation of resource: allowed*
*
* @param[in] portsName ID of ports_name
* @param[in] vlanid ID of vlanid
*
* Responses:
*
*/
void
Bridge::delete_bridge_ports_trunk_allowed_by_id(const std::string &portsName, const std::string &vlanid) {
  auto &&p = get_bridge_port(portsName);
  p.remove_trunk_vlan(std::stoul(vlanid));
}

/**
* @brief   Delete allowed by ID
*
* Delete operation of resource: allowed*
*
* @param[in] portsName ID of ports_name
*
* Responses:
*
*/
void
Bridge::delete_bridge_ports_trunk_allowed_list_by_id(const std::string &portsName) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Delete trunk by ID
*
* Delete operation of resource: trunk*
*
* @param[in] portsName ID of ports_name
*
* Responses:
*
*/
void
Bridge::delete_bridge_ports_trunk_by_id(const std::string &portsName) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Delete stp by ID
*
* Delete operation of resource: stp*
*
* @param[in] vlan ID of vlan
*
* Responses:
*
*/
void
Bridge::delete_bridge_stp_by_id(const std::string &vlan) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Delete stp by ID
*
* Delete operation of resource: stp*
*
*
* Responses:
*
*/
void
Bridge::delete_bridge_stp_list_by_id() {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Read agingtime by ID
*
* Read operation of resource: agingtime*
*
*
* Responses:
* int32_t
*/
int32_t
Bridge::read_bridge_agingtime_by_id() {
  return aging_time_;
}

/**
* @brief   Read bridge by ID
*
* Read operation of resource: bridge*
*
*
* Responses:
* BridgeSchema
*/
BridgeSchema
Bridge::read_bridge_by_id() {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Read age by ID
*
* Read operation of resource: age*
*
* @param[in] vlan ID of vlan
* @param[in] address ID of address
*
* Responses:
* int32_t
*/
int32_t
Bridge::read_bridge_filteringdatabase_age_by_id(const std::string &vlan, const std::string &address) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Read filteringdatabase by ID
*
* Read operation of resource: filteringdatabase*
*
* @param[in] vlan ID of vlan
* @param[in] address ID of address
*
* Responses:
* FilteringdatabaseSchema
*/
FilteringdatabaseSchema
Bridge::read_bridge_filteringdatabase_by_id(const std::string &vlan, const std::string &address) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Read entrytype by ID
*
* Read operation of resource: entrytype*
*
* @param[in] vlan ID of vlan
* @param[in] address ID of address
*
* Responses:
* std::string
*/
std::string
Bridge::read_bridge_filteringdatabase_entrytype_by_id(const std::string &vlan, const std::string &address) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Read filteringdatabase by ID
*
* Read operation of resource: filteringdatabase*
*
*
* Responses:
* std::vector<FilteringdatabaseSchema>
*/
std::vector<FilteringdatabaseSchema>
Bridge::read_bridge_filteringdatabase_list_by_id() {
  std::vector<FilteringdatabaseSchema> res;
  auto fwdtable = get_hash_table<fwd_key, fwd_entry>("fwdtable");
  auto entries = fwdtable.get_all();

  struct timespec now_;
  clock_gettime(CLOCK_MONOTONIC, &now_);
  const uint64_t SEC2NANOSEC = 1000000000ULL;
  uint64_t now = now_.tv_sec*SEC2NANOSEC + now_.tv_nsec;
  for (auto &it : entries) {
    FilteringdatabaseSchema fw;
    auto key = it.first;
    auto value = it.second;

    if ((now - value.timestamp) > aging_time_*SEC2NANOSEC) {
      logger()->debug("Ignoring old entry: now {0}, timestamp: {1}", now, value.timestamp);
       fwdtable.remove(key);
      continue;
    }
    fw.setAge((now - value.timestamp)/SEC2NANOSEC);

    // get vlan
    fw.setVlan(key.vlan);

    // get mac
    fw.setAddress(utils::be_uint_to_mac_string(key.mac));

    // get port
    auto p = get_port(value.port);
    fw.setPort(p->name());

    res.emplace_back(fw);
  }

  return res;
}

/**
* @brief   Read port by ID
*
* Read operation of resource: port*
*
* @param[in] vlan ID of vlan
* @param[in] address ID of address
*
* Responses:
* std::string
*/
std::string
Bridge::read_bridge_filteringdatabase_port_by_id(const std::string &vlan, const std::string &address) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Read access by ID
*
* Read operation of resource: access*
*
* @param[in] portsName ID of ports_name
*
* Responses:
* PortsAccessSchema
*/
PortsAccessSchema
Bridge::read_bridge_ports_access_by_id(const std::string &portsName) {
  PortsAccessSchema ports_access;
  auto &&p = get_bridge_port(portsName);
  ports_access.setVlanid(p.access_vlan());
  return ports_access;
}

/**
* @brief   Read vlanid by ID
*
* Read operation of resource: vlanid*
*
* @param[in] portsName ID of ports_name
*
* Responses:
* int32_t
*/
int32_t
Bridge::read_bridge_ports_access_vlanid_by_id(const std::string &portsName) {
  auto &&p = get_bridge_port(portsName);
  return p.access_vlan();
}

/**
* @brief   Read address by ID
*
* Read operation of resource: address*
*
* @param[in] portsName ID of ports_name
*
* Responses:
* std::string
*/
std::string
Bridge::read_bridge_ports_address_by_id(const std::string &portsName) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Read mode by ID
*
* Read operation of resource: mode*
*
* @param[in] portsName ID of ports_name
*
* Responses:
* std::string
*/
std::string
Bridge::read_bridge_ports_mode_by_id(const std::string &portsName) {
  auto &&p = get_bridge_port(portsName);
  return mode_to_string(p.mode());
}

/**
* @brief   Read stp by ID
*
* Read operation of resource: stp*
*
* @param[in] portsName ID of ports_name
* @param[in] vlan ID of vlan
*
* Responses:
* PortsStpSchema
*/
PortsStpSchema
Bridge::read_bridge_ports_stp_by_id(const std::string &portsName, const std::string &vlan) {
  auto &&stp = get_stp_instance(std::stoul(vlan));
  auto && p = get_bridge_port(portsName);
  PortsStpSchema port;
  port.setState(stp.get_port_state_str(p.index()));
  port.setVlan(std::stoul(vlan));
  // TODO: pathcost and priority
  return port;
}

/**
* @brief   Read stp by ID
*
* Read operation of resource: stp*
*
* @param[in] portsName ID of ports_name
*
* Responses:
* std::vector<PortsStpSchema>
*/
std::vector<PortsStpSchema>
Bridge::read_bridge_ports_stp_list_by_id(const std::string &portsName) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Read pathcost by ID
*
* Read operation of resource: pathcost*
*
* @param[in] portsName ID of ports_name
* @param[in] vlan ID of vlan
*
* Responses:
* int32_t
*/
int32_t
Bridge::read_bridge_ports_stp_pathcost_by_id(const std::string &portsName, const std::string &vlan) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Read portpriority by ID
*
* Read operation of resource: portpriority*
*
* @param[in] portsName ID of ports_name
* @param[in] vlan ID of vlan
*
* Responses:
* int32_t
*/
int32_t
Bridge::read_bridge_ports_stp_portpriority_by_id(const std::string &portsName, const std::string &vlan) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Read state by ID
*
* Read operation of resource: state*
*
* @param[in] portsName ID of ports_name
* @param[in] vlan ID of vlan
*
* Responses:
* std::string
*/
std::string
Bridge::read_bridge_ports_stp_state_by_id(const std::string &portsName, const std::string &vlan) {
  auto &&stp = get_stp_instance(std::stoul(vlan));
  auto && p = get_bridge_port(portsName);
  return stp.get_port_state_str(p.index());
}

/**
* @brief   Read allowed by ID
*
* Read operation of resource: allowed*
*
* @param[in] portsName ID of ports_name
* @param[in] vlanid ID of vlanid
*
* Responses:
* PortsTrunkAllowedSchema
*/
PortsTrunkAllowedSchema
Bridge::read_bridge_ports_trunk_allowed_by_id(const std::string &portsName, const std::string &vlanid) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Read allowed by ID
*
* Read operation of resource: allowed*
*
* @param[in] portsName ID of ports_name
*
* Responses:
* std::vector<PortsTrunkAllowedSchema>
*/
std::vector<PortsTrunkAllowedSchema>
Bridge::read_bridge_ports_trunk_allowed_list_by_id(const std::string &portsName) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Read trunk by ID
*
* Read operation of resource: trunk*
*
* @param[in] portsName ID of ports_name
*
* Responses:
* PortsTrunkSchema
*/
PortsTrunkSchema
Bridge::read_bridge_ports_trunk_by_id(const std::string &portsName) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Read nativevlan by ID
*
* Read operation of resource: nativevlan*
*
* @param[in] portsName ID of ports_name
*
* Responses:
* int32_t
*/
int32_t
Bridge::read_bridge_ports_trunk_nativevlan_by_id(const std::string &portsName) {
  auto && p = get_bridge_port(portsName);
  return p.native_vlan();
}

/**
* @brief   Read address by ID
*
* Read operation of resource: address*
*
* @param[in] vlan ID of vlan
*
* Responses:
* std::string
*/
std::string
Bridge::read_bridge_stp_address_by_id(const std::string &vlan) {
  auto &&stp = get_stp_instance(std::stoul(vlan));
  return stp.get_address();
}

/**
* @brief   Read stp by ID
*
* Read operation of resource: stp*
*
* @param[in] vlan ID of vlan
*
* Responses:
* StpSchema
*/
StpSchema
Bridge::read_bridge_stp_by_id(const std::string &vlan) {
  auto &&stp = get_stp_instance(std::stoul(vlan));
  StpSchema stpSchema;
  stpSchema.setVlan(std::stoul(vlan));
  stpSchema.setHellotime(stp.get_hello_time());
  stpSchema.setForwarddelay(stp.get_forward_delay());
  stpSchema.setPriority(stp.get_priority());
  stpSchema.setMaxmessageage(stp.get_max_age());
  stpSchema.setAddress(stp.get_address());
  return stpSchema;
}

/**
* @brief   Read forwarddelay by ID
*
* Read operation of resource: forwarddelay*
*
* @param[in] vlan ID of vlan
*
* Responses:
* int32_t
*/
int32_t
Bridge::read_bridge_stp_forwarddelay_by_id(const std::string &vlan) {
  auto &&stp = get_stp_instance(std::stoul(vlan));
  return stp.get_forward_delay();
}

/**
* @brief   Read hellotime by ID
*
* Read operation of resource: hellotime*
*
* @param[in] vlan ID of vlan
*
* Responses:
* int32_t
*/
int32_t
Bridge::read_bridge_stp_hellotime_by_id(const std::string &vlan) {
  auto &&stp = get_stp_instance(std::stoul(vlan));
  return stp.get_hello_time();
}

/**
* @brief   Read stp by ID
*
* Read operation of resource: stp*
*
*
* Responses:
* std::vector<StpSchema>
*/
std::vector<StpSchema>
Bridge::read_bridge_stp_list_by_id() {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Read maxmessageage by ID
*
* Read operation of resource: maxmessageage*
*
* @param[in] vlan ID of vlan
*
* Responses:
* int32_t
*/
int32_t
Bridge::read_bridge_stp_maxmessageage_by_id(const std::string &vlan) {
  auto &&stp = get_stp_instance(std::stoul(vlan));
  return stp.get_max_age();
}

/**
* @brief   Read priority by ID
*
* Read operation of resource: priority*
*
* @param[in] vlan ID of vlan
*
* Responses:
* int32_t
*/
int32_t
Bridge::read_bridge_stp_priority_by_id(const std::string &vlan) {
  auto &&stp = get_stp_instance(std::stoul(vlan));
  return stp.get_priority();
}

/**
* @brief   Read type by ID
*
* Read operation of resource: type*
*
*
* Responses:
* std::string
*/
std::string
Bridge::read_bridge_type_by_id() {
    CubeType type = get_type();

    switch(type) {
      case CubeType::TC:
        return std::string("TC");
      case CubeType::XDP_SKB:
      case CubeType::XDP_DRV:
        return std::string("TYPE_XDP");
    }

    return std::string("TYPE_UNKNOWN");
}

/**
* @brief   Read stpenabled by ID
*
* Read operation of resource: stpenabled*
*
*
* Responses:
* bool
*/
bool
Bridge::read_bridge_stpenabled_by_id() {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Update agingtime by ID
*
* Update operation of resource: agingtime*
*
* @param[in] agingtime Aging time of the filtering database
*
* Responses:
*
*/
void
Bridge::update_bridge_agingtime_by_id(const int32_t &agingtime) {
  // avoid unnecessary updates
  if (aging_time_ == agingtime)
    return;
  aging_time_ = agingtime;
  reload(generate_code(stp_enabled_));
}

/**
* @brief   Update bridge by ID
*
* Update operation of resource: bridge*
*
* @param[in] bridge bridgebody object
*
* Responses:
*
*/
void
Bridge::update_bridge_by_id(const BridgeSchema &bridge) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Update filteringdatabase by ID
*
* Update operation of resource: filteringdatabase*
*
* @param[in] vlan ID of vlan
* @param[in] address ID of address
* @param[in] filteringdatabase filteringdatabasebody object
*
* Responses:
*
*/
void
Bridge::update_bridge_filteringdatabase_by_id(const std::string &vlan, const std::string &address, const FilteringdatabaseSchema &filteringdatabase) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Update entrytype by ID
*
* Update operation of resource: entrytype*
*
* @param[in] vlan ID of vlan
* @param[in] address ID of address
* @param[in] entrytype Type of filtering entry
*
* Responses:
*
*/
void
Bridge::update_bridge_filteringdatabase_entrytype_by_id(const std::string &vlan, const std::string &address, const std::string &entrytype) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Update filteringdatabase by ID
*
* Update operation of resource: filteringdatabase*
*
* @param[in] filteringdatabase filteringdatabasebody object
*
* Responses:
*
*/
void
Bridge::update_bridge_filteringdatabase_list_by_id(const std::vector<FilteringdatabaseSchema> &filteringdatabase) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Update port by ID
*
* Update operation of resource: port*
*
* @param[in] vlan ID of vlan
* @param[in] address ID of address
* @param[in] port Output port name
*
* Responses:
*
*/
void
Bridge::update_bridge_filteringdatabase_port_by_id(const std::string &vlan, const std::string &address, const std::string &port) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Update access by ID
*
* Update operation of resource: access*
*
* @param[in] portsName ID of ports_name
* @param[in] access accessbody object
*
* Responses:
*
*/
void
Bridge::update_bridge_ports_access_by_id(const std::string &portsName, const PortsAccessSchema &access) {
  auto &&p = get_bridge_port(portsName);
  p.set_access_vlan(access.getVlanid());
}

/**
* @brief   Update vlanid by ID
*
* Update operation of resource: vlanid*
*
* @param[in] portsName ID of ports_name
* @param[in] vlanid VLAN associated with this interface
*
* Responses:
*
*/
void
Bridge::update_bridge_ports_access_vlanid_by_id(const std::string &portsName, const int32_t &vlanid) {
  auto &&p = get_bridge_port(portsName);
  p.set_access_vlan(vlanid);
}

/**
* @brief   Update address by ID
*
* Update operation of resource: address*
*
* @param[in] portsName ID of ports_name
* @param[in] address MAC address of the port
*
* Responses:
*
*/
void
Bridge::update_bridge_ports_address_by_id(const std::string &portsName, const std::string &address) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Update mode by ID
*
* Update operation of resource: mode*
*
* @param[in] portsName ID of ports_name
* @param[in] mode Type of bridge interface: access/trunk
*
* Responses:
*
*/
void
Bridge::update_bridge_ports_mode_by_id(const std::string &portsName, const std::string &mode) {
  auto &&p = get_bridge_port(portsName);
  PortMode requested_mode = string_to_mode(mode);
  p.set_mode(requested_mode);
}

/**
* @brief   Update stp by ID
*
* Update operation of resource: stp*
*
* @param[in] portsName ID of ports_name
* @param[in] vlan ID of vlan
* @param[in] stp stpbody object
*
* Responses:
*
*/
void
Bridge::update_bridge_ports_stp_by_id(const std::string &portsName, const std::string &vlan, const PortsStpSchema &stp) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Update stp by ID
*
* Update operation of resource: stp*
*
* @param[in] portsName ID of ports_name
* @param[in] stp stpbody object
*
* Responses:
*
*/
void
Bridge::update_bridge_ports_stp_list_by_id(const std::string &portsName, const std::vector<PortsStpSchema> &stp) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Update pathcost by ID
*
* Update operation of resource: pathcost*
*
* @param[in] portsName ID of ports_name
* @param[in] vlan ID of vlan
* @param[in] pathcost STP cost associated with this interface
*
* Responses:
*
*/
void
Bridge::update_bridge_ports_stp_pathcost_by_id(const std::string &portsName, const std::string &vlan, const int32_t &pathcost) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Update portpriority by ID
*
* Update operation of resource: portpriority*
*
* @param[in] portsName ID of ports_name
* @param[in] vlan ID of vlan
* @param[in] portpriority Port priority of this interface
*
* Responses:
*
*/
void
Bridge::update_bridge_ports_stp_portpriority_by_id(const std::string &portsName, const std::string &vlan, const int32_t &portpriority) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Update allowed by ID
*
* Update operation of resource: allowed*
*
* @param[in] portsName ID of ports_name
* @param[in] vlanid ID of vlanid
* @param[in] allowed allowedbody object
*
* Responses:
*
*/
void
Bridge::update_bridge_ports_trunk_allowed_by_id(const std::string &portsName, const std::string &vlanid, const PortsTrunkAllowedSchema &allowed) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Update allowed by ID
*
* Update operation of resource: allowed*
*
* @param[in] portsName ID of ports_name
* @param[in] allowed allowedbody object
*
* Responses:
*
*/
void
Bridge::update_bridge_ports_trunk_allowed_list_by_id(const std::string &portsName, const std::vector<PortsTrunkAllowedSchema> &allowed) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Update trunk by ID
*
* Update operation of resource: trunk*
*
* @param[in] portsName ID of ports_name
* @param[in] trunk trunkbody object
*
* Responses:
*
*/
void
Bridge::update_bridge_ports_trunk_by_id(const std::string &portsName, const PortsTrunkSchema &trunk) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Update nativevlan by ID
*
* Update operation of resource: nativevlan*
*
* @param[in] portsName ID of ports_name
* @param[in] nativevlan VLAN that is not tagged in this trunk port
*
* Responses:
*
*/
void
Bridge::update_bridge_ports_trunk_nativevlan_by_id(const std::string &portsName, const int32_t &nativevlan) {
  logger()->info("Updating native vlan!");
  auto &&p = get_bridge_port(portsName);
  p.set_native_vlan(nativevlan);
  logger()->info("native vlan updated!");
}

/**
* @brief   Update address by ID
*
* Update operation of resource: address*
*
* @param[in] vlan ID of vlan
* @param[in] address Main MAC address used by the STP
*
* Responses:
*
*/
void
Bridge::update_bridge_stp_address_by_id(const std::string &vlan, const std::string &address) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Update stp by ID
*
* Update operation of resource: stp*
*
* @param[in] vlan ID of vlan
* @param[in] stp stpbody object
*
* Responses:
*
*/
void
Bridge::update_bridge_stp_by_id(const std::string &vlan, const StpSchema &stp) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Update forwarddelay by ID
*
* Update operation of resource: forwarddelay*
*
* @param[in] vlan ID of vlan
* @param[in] forwarddelay Delay used by STP bridges for port state transition
*
* Responses:
*
*/
void
Bridge::update_bridge_stp_forwarddelay_by_id(const std::string &vlan, const int32_t &forwarddelay) {
  auto &&stp = get_stp_instance(std::stoul(vlan));
  stp.set_forward_delay(forwarddelay);
}

/**
* @brief   Update hellotime by ID
*
* Update operation of resource: hellotime*
*
* @param[in] vlan ID of vlan
* @param[in] hellotime Interval between transmissions of BPDU messages
*
* Responses:
*
*/
void
Bridge::update_bridge_stp_hellotime_by_id(const std::string &vlan, const int32_t &hellotime) {
  auto &&stp = get_stp_instance(std::stoul(vlan));
  stp.set_hello_time(hellotime);
}

/**
* @brief   Update stp by ID
*
* Update operation of resource: stp*
*
* @param[in] stp stpbody object
*
* Responses:
*
*/
void
Bridge::update_bridge_stp_list_by_id(const std::vector<StpSchema> &stp) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Update maxmessageage by ID
*
* Update operation of resource: maxmessageage*
*
* @param[in] vlan ID of vlan
* @param[in] maxmessageage Maximum age of a BPDU
*
* Responses:
*
*/
void
Bridge::update_bridge_stp_maxmessageage_by_id(const std::string &vlan, const int32_t &maxmessageage) {
  auto &&stp = get_stp_instance(std::stoul(vlan));
  stp.set_max_age(maxmessageage);
}

/**
* @brief   Update priority by ID
*
* Update operation of resource: priority*
*
* @param[in] vlan ID of vlan
* @param[in] priority Bridge priority for STP
*
* Responses:
*
*/
void
Bridge::update_bridge_stp_priority_by_id(const std::string &vlan, const int32_t &priority) {
  auto &&stp = get_stp_instance(std::stoul(vlan));
  stp.set_priority(priority);
}

/**
* @brief   Update stpenabled by ID
*
* Update operation of resource: stpenabled*
*
* @param[in] stpenabled Enable/Disable the STP protocol of the bridge
*
* Responses:
*
*/
void
Bridge::update_bridge_stpenabled_by_id(const bool &stpenabled) {
    throw std::runtime_error("Method not implemented");
}

/**
* @brief   Update type by ID
*
* Update operation of resource: type*
*
* @param[in] type Type of the Cube (TC, XDP_SKB, XDP_DRV)
*
* Responses:
*
*/
void
Bridge::update_bridge_type_by_id(const std::string &type) {
    throw std::runtime_error("You cannot change the CubeType at runtime");
}
