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

#include "../interface/FdbEntryInterface.h"

#include <spdlog/spdlog.h>

class Fdb;

/* definitions copied from datapath */
struct fwd_entry {
  uint32_t timestamp;
  uint32_t port;
} __attribute__((packed));

using namespace io::swagger::server::model;

class FdbEntry : public FdbEntryInterface {
 public:
  FdbEntry(Fdb &parent, const FdbEntryJsonObject &conf);
  FdbEntry(Fdb &parent, const std::string &address, uint32_t entry_age,
           uint32_t out_port);
  virtual ~FdbEntry();

  static void create(Fdb &parent, const std::string &address,
                     const FdbEntryJsonObject &conf);
  static std::shared_ptr<FdbEntry> getEntry(Fdb &parent,
                                            const std::string &address);
  static void removeEntry(Fdb &parent, const std::string &address);
  static std::vector<std::shared_ptr<FdbEntry>> get(Fdb &parent);
  static void remove(Fdb &parent);
  std::shared_ptr<spdlog::logger> logger();
  void update(const FdbEntryJsonObject &conf) override;
  FdbEntryJsonObject toJsonObject() override;

  /// <summary>
  /// Age of the current filtering database entry
  /// </summary>
  uint32_t getAge() override;

  /// <summary>
  /// Output port name
  /// </summary>
  std::string getPort() override;
  void setPort(const std::string &value) override;

  /// <summary>
  /// Address of the filtering database entry
  /// </summary>
  std::string getAddress() override;

 private:
  Fdb &parent_;

  /*
   * tries to construct a FdbEntry from map data, it checks the age of the
   * entry as well as if the port is still valid
   */
  static std::shared_ptr<FdbEntry> constructFromMap(Fdb &parent,
                                                    const std::string &key,
                                                    const fwd_entry &value);

  std::string address_;
  std::string port_name_;
  uint32_t entry_age_;
  uint32_t port_no_;
};
