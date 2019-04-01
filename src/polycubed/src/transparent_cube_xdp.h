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

#include "transparent_cube.h"

#include <api/BPF.h>
#include <api/BPFTable.h>

#include <spdlog/spdlog.h>

#include <exception>
#include <map>
#include <set>
#include <vector>

namespace polycube {
namespace polycubed {

class TransparentCubeXDP : virtual public TransparentCube {
 public:
  explicit TransparentCubeXDP(const std::string &name,
                              const std::string &service_name,
                              const std::vector<std::string> &ingres_code,
                              const std::vector<std::string> &egress_code,
                              LogLevel level, CubeType type,
                              const service::attach_cb &attach);
  virtual ~TransparentCubeXDP();

 protected:
  static std::string get_wrapper_code();

  void compile(ebpf::BPF &bpf, const std::string &code, int index,
               ProgramType type);
  int load(ebpf::BPF &bpf, ProgramType type);
  void unload(ebpf::BPF &bpf, ProgramType type);
  void compileIngress(ebpf::BPF &bpf, const std::string &code, uint16_t next);
  void compileEgress(ebpf::BPF &bpf, const std::string &code, uint16_t next);

 private:
  static const std::string TRANSPARENTCUBEXDP_WRAPPER;
};

}  // namespace polycubed
}  // namespace polycube
