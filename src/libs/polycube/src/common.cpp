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

#include "polycube/common.h"

#include <api/BPFTable.h>

namespace polycube {

static std::string level_names[] \
  {"TRACE", "DEBUG", "INFO", "WARN", "ERR", "CRITICAL", "OFF"};

std::string logLevelString(polycube::LogLevel l) {
  return level_names[(int)l];
}

polycube::LogLevel stringLogLevel(const std::string &level) {
  for (int i = 0; i < sizeof(level_names) / sizeof(level_names[0]); i++) {
    if (level_names[i] == level)
      return static_cast<polycube::LogLevel>(i);
  }
}

spdlog::level::level_enum logLevelToSPDLog(polycube::LogLevel level) {
  switch (level) {
    case polycube::LogLevel::TRACE: return spdlog::level::trace;
    case polycube::LogLevel::DEBUG: return spdlog::level::debug;
    case polycube::LogLevel::INFO: return spdlog::level::info;
    case polycube::LogLevel::WARN: return spdlog::level::warn;
    case polycube::LogLevel::ERR: return spdlog::level::err;
    case polycube::LogLevel::CRITICAL: return spdlog::level::critical;
    case polycube::LogLevel::OFF: return spdlog::level::off;
  }
}

size_t get_possible_cpu_count() {
  return ebpf::BPFTable::get_possible_cpu_count();
}

}  // namespace polycube
