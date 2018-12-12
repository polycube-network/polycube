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

#include <functional>
#include <thread>
#include <vector>

#include <api/BPF.h>
#include <spdlog/spdlog.h>

#include "polycube/services/cube_factory.h"

using polycube::service::LogMsg;
using polycube::service::log_msg_cb;

namespace polycube {
namespace polycubed {

class DatapathLog {
 public:
  static DatapathLog &get_instance();
  ~DatapathLog();
  void register_cb(int id, const log_msg_cb &cb);
  void unregister_cb(int id);

  void start();
  void stop();

  static void call_back_proxy(void *cb_cookie, void *data, int data_size);

  // replaces all the log calls to the code that does it
  std::string parse_log(const std::string &code);

 private:
  DatapathLog();
  std::unique_ptr<std::thread> dbg_thread_;
  ebpf::BPF perf_buffer_;
  std::map<uint32_t, const log_msg_cb &> cbs_;
  std::mutex cbs_mutex_; // protects the cbs_ container
  std::shared_ptr<spdlog::logger> logger;
  bool stop_;
  std::string base_code_;

};

}  // namespace polycubed
}  // namespace polycube
