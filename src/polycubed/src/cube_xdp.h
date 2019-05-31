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

#include "cube.h"
#include "node.h"
#include "polycube/services/guid.h"

#include <api/BPF.h>
#include <api/BPFTable.h>

#include <spdlog/spdlog.h>
#include "polycube/services/cube_factory.h"

#include <exception>
#include <map>
#include <set>
#include <vector>

using polycube::service::PortIface;

namespace polycube {
namespace polycubed {

class PortXDP;
class TransparentCubeXDP;

class CubeXDP : public Cube {
  friend class PortXDP;
  friend class TransparentCubeXDP;

 public:
  explicit CubeXDP(const std::string &name, const std::string &service_name,
                   const std::vector<std::string> &ingress_code,
                   const std::vector<std::string> &egress_code, LogLevel level,
                   CubeType type, bool shadow, bool span);

  virtual ~CubeXDP();
  int get_attach_flags() const;

 protected:
  static std::string get_wrapper_code();

  void compile(ebpf::BPF &bpf, const std::string &code, int index,
               ProgramType type);
  int load(ebpf::BPF &bpf, ProgramType type);
  void unload(ebpf::BPF &bpf, ProgramType type);

  static void do_unload(ebpf::BPF &bpf);
  static int do_load(ebpf::BPF &bpf);
  void compileIngress(ebpf::BPF &bpf, const std::string &code);
  void compileEgress(ebpf::BPF &bpf, const std::string &code);

  int attach_flags_;

 private:
  static const std::string CUBEXDP_COMMON_WRAPPER;
  static const std::string CUBEXDP_WRAPPER;
  static const std::string CUBEXDP_HELPERS;
};

}  // namespace polycubed
}  // namespace polycube
