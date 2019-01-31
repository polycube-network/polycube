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

#pragma once

#include <map>
#include <string>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include "polycube/common.h"

#include "polycube/services/cube_factory.h"
#include "polycube/services/cube_iface.h"
#include "polycube/services/management_interface.h"
#include "polycube/services/table.h"
#include "polycube/services/utils.h"

namespace polycube {
namespace service {

extern "C" ServiceMetadata init(CubeFactory *factory, const char *logfile);
extern CubeFactory *factory_;
extern std::string logfile_;

class BaseCube {
 public:
  BaseCube(const std::string &name,
           const std::vector<std::string> &ingress_code,
           const std::vector<std::string> &egress_code, const CubeType type,
           LogLevel level = LogLevel::OFF);
  virtual ~BaseCube();

  // TODO: implement move constructor, forbid copy and asignment

  void reload(const std::string &code, int index = 0,
              ProgramType type = ProgramType::INGRESS);
  int add_program(const std::string &code, int index = -1,
                  ProgramType type = ProgramType::INGRESS);
  void del_program(int index, ProgramType type = ProgramType::INGRESS);

  // Accessors for tables
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

  virtual void datapath_log_msg(const LogMsg *msg);

  void set_log_level(LogLevel level);
  LogLevel get_log_level() const;

  const Guid &get_uuid() const;
  const std::string get_name() const;
  CubeType get_type() const;

  // protected: (later on)
  std::shared_ptr<spdlog::logger> logger();

  void dismount();

 protected:
  int get_table_fd(const std::string &table_name, int index, ProgramType type);

  std::shared_ptr<BaseCubeIface> cube_;  // pointer to the cube in polycubed
  log_msg_cb handle_log_msg;
  CubeType type_;
  std::shared_ptr<spdlog::logger> logger_;
  std::atomic<bool> dismounted_;

  std::mutex cube_mutex;
};

template <class ValueType>
ArrayTable<ValueType> BaseCube::get_array_table(const std::string &table_name,
                                                int index, ProgramType type) {
  int fd = get_table_fd(table_name, index, type);
  return ArrayTable<ValueType>(&fd);
};

template <class ValueType>
PercpuArrayTable<ValueType> BaseCube::get_percpuarray_table(
    const std::string &table_name, int index, ProgramType type) {
  int fd = get_table_fd(table_name, index, type);
  return PercpuArrayTable<ValueType>(&fd);
  ;
}

template <class KeyType, class ValueType>
HashTable<KeyType, ValueType> BaseCube::get_hash_table(
    const std::string &table_name, int index, ProgramType type) {
  int fd = get_table_fd(table_name, index, type);
  return HashTable<KeyType, ValueType>(&fd);
}

template <class KeyType, class ValueType>
PercpuHashTable<KeyType, ValueType> BaseCube::get_percpuhash_table(
    const std::string &table_name, int index, ProgramType type) {
  int fd = get_table_fd(table_name, index, type);
  return PercpuHashTable<KeyType, ValueType>(&fd);
}

}  // namespace service
}  // namespace polycube
