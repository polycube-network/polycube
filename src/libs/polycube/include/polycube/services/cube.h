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

#include <map>
#include <string>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include "polycube/common.h"

#include "polycube/services/cube_factory.h"
#include "polycube/services/management_interface.h"
#include "polycube/services/port.h"
#include "polycube/services/table.h"
#include "polycube/services/utils.h"

namespace polycube {
namespace service {

extern "C" ServiceMetadata init(CubeFactory *factory, std::string logfile);
extern CubeFactory *factory_;
extern std::string logfile_;

template <class PortType>
class Cube {
  friend ServiceMetadata init(CubeFactory *factory);

 public:
  Cube(const std::string &name, const std::vector<std::string> &ingress_code,
       const std::vector<std::string> &egress_code, const CubeType type,
       LogLevel level = LogLevel::OFF);
  virtual ~Cube();

  // TODO: implement move constructor, forbid copy and asignment

  void reload(const std::string &code, int index = 0,
              ProgramType type = ProgramType::INGRESS);
  int add_program(const std::string &code, int index = -1,
                  ProgramType type = ProgramType::INGRESS);
  void del_program(int index, ProgramType type = ProgramType::INGRESS);

  template <class PortConfigType>
  std::shared_ptr<PortType> add_port(const std::string &port_name,
                                     const PortConfigType &conf);
  void remove_port(const std::string &port_name);
  std::shared_ptr<PortType> get_port(const std::string &port_name);
  std::shared_ptr<PortType> get_port(int index);
  std::vector<std::shared_ptr<PortType>> get_ports();

  /* Accessors for tables */
  RawTable get_raw_table(const std::string &table_name, int index = 0,
                         ProgramType type = ProgramType::INGRESS);
  template <class ValueType>
  ArrayTable<ValueType> get_array_table(
      const std::string &table_name, int index = 0,
      ProgramType type = ProgramType::INGRESS);
  template <class ValueType>
  PercpuArrayTable<ValueType> get_percpuarray_table(
      const std::string &table_name, int index = 0,
      ProgramType type = ProgramType::INGRESS);
  template <class KeyType, class ValueType>
  HashTable<KeyType, ValueType> get_hash_table(
      const std::string &table_name, int index = 0,
      ProgramType type = ProgramType::INGRESS);
  template <class KeyType, class ValueType>
  PercpuHashTable<KeyType, ValueType> get_percpuhash_table(
      const std::string &table_name, int index = 0,
      ProgramType type = ProgramType::INGRESS);

  virtual void packet_in(PortType &port, PacketInMetadata &md,
                         const std::vector<uint8_t> &packet) = 0;

  virtual void datapath_log_msg(const LogMsg *msg);

  void set_log_level(LogLevel level);
  LogLevel get_log_level() const;

  const Guid &get_uuid() const;
  const std::string get_name() const;
  CubeType get_type() const;

  // protected: (later on)
  std::shared_ptr<spdlog::logger> logger();

  void dismount();

 private:
  class impl;
  std::unique_ptr<impl> pimpl_;
};

template <class PortType>
class Cube<PortType>::impl {
 private:
  int get_table_fd(const std::string &table_name, int index, ProgramType type);

 public:
  impl(Cube<PortType> &parent_, const std::string &name,
       const std::vector<std::string> &ingress_code,
       const std::vector<std::string> &egress_code, const CubeType type,
       LogLevel level);
  ~impl();
  void reload(const std::string &code, int index, ProgramType type);
  int add_program(const std::string &code, int index, ProgramType type);
  void del_program(int index, ProgramType type);

  template <class PortConfigType>
  std::shared_ptr<PortType> add_port(const std::string &port_name,
                                     const PortConfigType &conf);
  void remove_port(const std::string &port_name);
  std::shared_ptr<PortType> get_port(const std::string &port_name);
  std::shared_ptr<PortType> get_port(int index);
  std::vector<std::shared_ptr<PortType>> get_ports();

  RawTable get_raw_table(const std::string &table_name, int index,
                         ProgramType type);

  template <class ValueType>
  ArrayTable<ValueType> get_array_table(const std::string &table_name,
                                        int index, ProgramType type) {
    int fd = get_table_fd(table_name, index, type);
    return ArrayTable<ValueType>(&fd);
  };

  template <class ValueType>
  PercpuArrayTable<ValueType> get_percpuarray_table(
      const std::string &table_name, int index, ProgramType type) {
    int fd = get_table_fd(table_name, index, type);
    return PercpuArrayTable<ValueType>(&fd);
    ;
  }

  template <class KeyType, class ValueType>
  HashTable<KeyType, ValueType> get_hash_table(const std::string &table_name,
                                               int index, ProgramType type) {
    int fd = get_table_fd(table_name, index, type);
    return HashTable<KeyType, ValueType>(&fd);
  }

  template <class KeyType, class ValueType>
  PercpuHashTable<KeyType, ValueType> get_percpuhash_table(
      const std::string &table_name, int index, ProgramType type) {
    int fd = get_table_fd(table_name, index, type);
    return PercpuHashTable<KeyType, ValueType>(&fd);
  }

  void datapath_log_msg(const LogMsg *msg);

  void set_log_level(LogLevel level);
  LogLevel get_log_level() const;

  const Guid &get_uuid() const;
  const std::string get_name() const;
  CubeType get_type() const;

  std::shared_ptr<spdlog::logger> logger();

  void dismount();

 private:
  std::shared_ptr<CubeIface> cube_;  // pointer to the cube in polycubed
  Cube &parent_;                     // needed to call packet_in()
  packet_in_cb handle_packet_in;
  log_msg_cb handle_log_msg;
  CubeType type_;
  std::shared_ptr<spdlog::logger> logger_;
  std::atomic<bool> dismounted_;

  std::map<std::string, std::shared_ptr<PortType>> ports_by_name_;
  std::map<int, std::shared_ptr<PortType>> ports_by_id_;

  std::mutex cube_mutex;
};

template <class PortType>
Cube<PortType>::impl::impl(Cube<PortType> &parent_, const std::string &name,
                           const std::vector<std::string> &ingress_code,
                           const std::vector<std::string> &egress_code,
                           const CubeType type, LogLevel level)
    : parent_(parent_),
      type_(type),
      dismounted_(false),
      logger_(std::make_shared<spdlog::logger>(
          name, (spdlog::sinks_init_list){
                    std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                        logfile_, 1048576 * 5, 3),
                    std::make_shared<spdlog::sinks::stdout_sink_mt>()})) {
  logger()->set_level(logLevelToSPDLog(level));
  // TODO: move to function
  handle_packet_in = [&](const PacketIn *md,
                         const std::vector<uint8_t> &packet) -> void {
    // This lock guarantees:
    // - port is not deleted while processing it
    // - service implementation is not deleted wile processing it
    std::lock_guard<std::mutex> guard(cube_mutex);
    if (dismounted_)
      return;

    auto &p = *ports_by_id_.at(md->port_id);
    PacketInMetadata md_;
    md_.reason = md->reason;
    md_.metadata[0] = md->metadata[0];
    md_.metadata[1] = md->metadata[1];
    md_.metadata[2] = md->metadata[2];
    parent_.packet_in(p, md_, packet);
  };

  handle_log_msg = [&](const LogMsg *msg) -> void {
    parent_.datapath_log_msg(msg);
  };

  cube_ = factory_->create_cube(name, ingress_code, egress_code, handle_log_msg,
                                type, handle_packet_in, level);
}

template <class PortType>
Cube<PortType>::impl::~impl() {
  // just in case
  if (!dismounted_) {
    dismount();
  }

  factory_->destroy_cube(get_name());
}

template <class PortType>
int Cube<PortType>::impl::get_table_fd(const std::string &table_name, int index,
                                       ProgramType type) {
  return cube_->get_table_fd(table_name, index, type);
}

template <class PortType>
void Cube<PortType>::impl::reload(const std::string &code, int index,
                                  ProgramType type) {
  cube_->reload(code, index, type);
}

template <class PortType>
int Cube<PortType>::impl::add_program(const std::string &code, int index,
                                      ProgramType type) {
  return cube_->add_program(code, index, type);
}

template <class PortType>
void Cube<PortType>::impl::del_program(int index, ProgramType type) {
  cube_->del_program(index, type);
}

template <class PortType>
template <class PortConfigType>
std::shared_ptr<PortType> Cube<PortType>::impl::add_port(
    const std::string &port_name, const PortConfigType &conf) {
  if (ports_by_name_.count(port_name) != 0) {
    throw std::runtime_error("Port " + port_name + " already exists");
  }

  auto cube_port = cube_->add_port(port_name);

  try {
    typename std::map<std::string, std::shared_ptr<PortType>>::iterator iter;
    bool inserted;

    auto port = std::make_shared<PortType>(parent_, cube_port, conf);

    std::tie(iter, inserted) = ports_by_name_.emplace(port_name, port);
    ports_by_id_.emplace(port->index(), port);
    return iter->second;
  } catch (const std::exception &ex) {
    cube_->remove_port(port_name);
    throw;
  }
}

template <class PortType>
std::vector<std::shared_ptr<PortType>> Cube<PortType>::impl::get_ports() {
  std::vector<std::shared_ptr<PortType>> ports;
  for (auto &it : ports_by_name_)
    ports.push_back(it.second);

  return ports;
}

template <class PortType>
void Cube<PortType>::impl::remove_port(const std::string &port_name) {
  // avoid deleting a port while processing a packet_in event
  std::lock_guard<std::mutex> guard(cube_mutex);

  if (ports_by_name_.count(port_name) == 0) {
    throw std::runtime_error("Port " + port_name + " does not exist");
  }

  cube_->remove_port(port_name);
  auto id = ports_by_name_.at(port_name)->index();
  ports_by_name_.erase(port_name);
  ports_by_id_.erase(id);
}

template <class PortType>
std::shared_ptr<PortType> Cube<PortType>::impl::get_port(
    const std::string &port_name) {
  if (ports_by_name_.count(port_name) == 0) {
    throw std::runtime_error("Port " + port_name + " does not exist");
  }

  return ports_by_name_.at(port_name);
}

template <class PortType>
std::shared_ptr<PortType> Cube<PortType>::impl::get_port(int index) {
  if (ports_by_id_.count(index) == 0) {
    throw std::runtime_error("Port id " + std::to_string(index) +
                             " does not exist");
  }

  return ports_by_id_.at(index);
}

template <class PortType>
RawTable Cube<PortType>::impl::get_raw_table(const std::string &table_name,
                                             int index, ProgramType type) {
  int fd = get_table_fd(table_name, index, type);
  RawTable t(&fd);
  return std::move(t);
}

template <class PortType>
void Cube<PortType>::impl::datapath_log_msg(const LogMsg *msg) {
  spdlog::level::level_enum level_ =
      logLevelToSPDLog((polycube::LogLevel)msg->level);
  std::string print;

  switch (msg->type) {
  case 0:
    print = utils::format_debug_string(msg->msg, msg->args);
    logger()->log(level_, print.c_str());
    break;

  case 1:
#ifdef HAVE_POLYCUBE_TOOLS
    logger()->log(level_, "packet received for debug:");
    utils::print_packet((const uint8_t *)msg->msg, msg->len);
#else
    logger()->warn(
        "Received packet for debugging. polycube-tools is not available");
#endif
    break;

  default:
    logger()->warn("Received bad message type in datapath_log_msg");
    return;
  }
}

template <class PortType>
void Cube<PortType>::impl::set_log_level(LogLevel level) {
  logger()->set_level(logLevelToSPDLog(level));
  return cube_->set_log_level(level);
}

template <class PortType>
LogLevel Cube<PortType>::impl::get_log_level() const {
  return cube_->get_log_level();
}

template <class PortType>
const Guid &Cube<PortType>::impl::get_uuid() const {
  return cube_->uuid();
}

template <class PortType>
const std::string Cube<PortType>::impl::get_name() const {
  return cube_->get_name();
}

template <class PortType>
CubeType Cube<PortType>::impl::get_type() const {
  return type_;
}

template <class PortType>
std::shared_ptr<spdlog::logger> Cube<PortType>::impl::logger() {
  return logger_;
}

template <class PortType>
void Cube<PortType>::impl::dismount() {
  std::lock_guard<std::mutex> guard(cube_mutex);
  dismounted_ = true;
  // invalidate handlers
  handle_packet_in = nullptr;
  handle_log_msg = nullptr;

  // TODO: remove from controller and datapathlog?
}

// PIMPL
template <class PortType>
Cube<PortType>::Cube(const std::string &name,
                     const std::vector<std::string> &ingress_code,
                     const std::vector<std::string> &egress_code,
                     const CubeType type, LogLevel level)
    : pimpl_(new Cube::impl(*this, name, ingress_code, egress_code, type,
                            level)) {}

template <class PortType>
Cube<PortType>::~Cube() {}

template <class PortType>
void Cube<PortType>::reload(const std::string &code, int index,
                            ProgramType type) {
  return pimpl_->reload(code, index, type);
}

template <class PortType>
int Cube<PortType>::add_program(const std::string &code, int index,
                                ProgramType type) {
  return pimpl_->add_program(code, index, type);
}

template <class PortType>
void Cube<PortType>::del_program(int index, ProgramType type) {
  return pimpl_->del_program(index, type);
}

template <class PortType>
template <class PortConfigType>
std::shared_ptr<PortType> Cube<PortType>::add_port(const std::string &port_name,
                                                   const PortConfigType &conf) {
  return std::move(pimpl_->template add_port<PortConfigType>(port_name, conf));
}

template <class PortType>
void Cube<PortType>::remove_port(const std::string &port_name) {
  return pimpl_->remove_port(port_name);
}

template <class PortType>
std::shared_ptr<PortType> Cube<PortType>::get_port(
    const std::string &port_name) {
  return std::move(pimpl_->get_port(port_name));
}

template <class PortType>
std::shared_ptr<PortType> Cube<PortType>::get_port(int index) {
  return std::move(pimpl_->get_port(index));
}

template <class PortType>
std::vector<std::shared_ptr<PortType>> Cube<PortType>::get_ports() {
  return std::move(pimpl_->get_ports());
}

template <class PortType>
RawTable Cube<PortType>::get_raw_table(const std::string &table_name, int index,
                                       ProgramType type) {
  return pimpl_->get_raw_table(table_name, index, type);
}

template <class PortType>
void Cube<PortType>::set_log_level(LogLevel level) {
  return pimpl_->set_log_level(level);
}

template <class PortType>
LogLevel Cube<PortType>::get_log_level() const {
  return pimpl_->get_log_level();
}

template <class PortType>
const Guid &Cube<PortType>::get_uuid() const {
  return pimpl_->get_uuid();
}

template <class PortType>
void Cube<PortType>::datapath_log_msg(const LogMsg *msg) {
  return pimpl_->datapath_log_msg(msg);
}

template <class PortType>
const std::string Cube<PortType>::get_name() const {
  return pimpl_->get_name();
}

template <class PortType>
CubeType Cube<PortType>::get_type() const {
  return pimpl_->get_type();
}

template <class PortType>
std::shared_ptr<spdlog::logger> Cube<PortType>::logger() {
  return pimpl_->logger();
}

template <class PortType>
void Cube<PortType>::dismount() {
  return pimpl_->dismount();
}

template <class PortType>
template <class ValueType>
ArrayTable<ValueType> Cube<PortType>::get_array_table(
    const std::string &table_name, int index, ProgramType type) {
  return pimpl_->template get_array_table<ValueType>(table_name, index, type);
}
template <class PortType>
template <class ValueType>
PercpuArrayTable<ValueType> Cube<PortType>::get_percpuarray_table(
    const std::string &table_name, int index, ProgramType type) {
  return pimpl_->template get_percpuarray_table<ValueType>(table_name, index,
                                                           type);
}

template <class PortType>
template <class KeyType, class ValueType>
HashTable<KeyType, ValueType> Cube<PortType>::get_hash_table(
    const std::string &table_name, int index, ProgramType type) {
  return pimpl_->template get_hash_table<KeyType, ValueType>(table_name, index,
                                                             type);
}

template <class PortType>
template <class KeyType, class ValueType>
PercpuHashTable<KeyType, ValueType> Cube<PortType>::get_percpuhash_table(
    const std::string &table_name, int index, ProgramType type) {
  return pimpl_->template get_percpuhash_table<KeyType, ValueType>(table_name,
                                                                   index, type);
}

}  // namespace service
}  // namespace polycube
