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

class TransparentCubeXDP;

namespace polycube {
namespace polycubed {

class TransparentCubeTC : virtual public TransparentCube {
  friend class TransparentCubeXDP;

 public:
  explicit TransparentCubeTC(const std::string &name,
                             const std::string &service_name,
                             const std::vector<std::string> &ingres_code,
                             const std::vector<std::string> &egress_code,
                             LogLevel level, const service::attach_cb &attach);
  virtual ~TransparentCubeTC();

 protected:
  static void do_compile(int module_index, uint16_t next, ProgramType type,
                         LogLevel level_, ebpf::BPF &bpf,
                         const std::string &code, int index);
  static std::string get_wrapper_code();

  void compile(ebpf::BPF &bpf, const std::string &code, int index,
               ProgramType type);
  int load(ebpf::BPF &bpf, ProgramType type);
  void unload(ebpf::BPF &bpf, ProgramType type);

 private:
  static const std::string TRANSPARENTCUBETC_WRAPPER;
};

}  // namespace polycubed
}  // namespace polycube
