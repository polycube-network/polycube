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

#include "../base/PortsBase.h"

class Simplebridge;

using namespace polycube::service::model;

class Ports : public PortsBase {
 public:
  Ports(polycube::service::Cube<Ports> &parent,
      std::shared_ptr<polycube::service::PortIface> port,
      const PortsJsonObject &conf);
  virtual ~Ports();

  /// <summary>
  /// MAC address of the port
  /// </summary>
  std::string getMac() override;

 private:
  std::string mac_;
};
