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

#include "extiface.h"
#include "port_tc.h"

#include <api/BPF.h>
#include <spdlog/spdlog.h>

#include <stdio.h>

#include <set>

namespace polycube {
namespace polycubed {

class PortTC;
class ExtIface;
class ExtIfaceXDP;

class ExtIfaceTC : public ExtIface {
  friend class ExtIface;
  friend class ExtIfaceXDP;

 public:
  ExtIfaceTC(const std::string &iface, const std::string &node="");
  virtual ~ExtIfaceTC();

 protected:
  int load_ingress();

  virtual std::string get_ingress_code();
  virtual std::string get_egress_code();
  virtual bpf_prog_type get_program_type() const;

  static const std::string RX_CODE;
};

}  // namespace polycubed
}  // namespace polycube
