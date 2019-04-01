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
#include "polycube/services/cube_iface.h"
#include "polycube/services/port_iface.h"

#include <api/BPFTable.h>

namespace polycube {

// https://stackoverflow.com/a/4119881
bool iequals(const std::string &a, const std::string &b) {
  unsigned int sz = a.size();
  if (b.size() != sz)
    return false;
  for (unsigned int i = 0; i < sz; ++i)
    if (tolower(a[i]) != tolower(b[i]))
      return false;
  return true;
}

static std::string level_names[]{"TRACE", "DEBUG",    "INFO", "WARN",
                                 "ERR",   "CRITICAL", "OFF"};

std::string logLevelString(polycube::LogLevel l) {
  return level_names[(int)l];
}

polycube::LogLevel stringLogLevel(const std::string &level) {
  for (int i = 0; i < sizeof(level_names) / sizeof(level_names[0]); i++) {
    if (iequals(level_names[i], level))
      return static_cast<polycube::LogLevel>(i);
  }
}

spdlog::level::level_enum logLevelToSPDLog(polycube::LogLevel level) {
  switch (level) {
  case polycube::LogLevel::TRACE:
    return spdlog::level::trace;
  case polycube::LogLevel::DEBUG:
    return spdlog::level::debug;
  case polycube::LogLevel::INFO:
    return spdlog::level::info;
  case polycube::LogLevel::WARN:
    return spdlog::level::warn;
  case polycube::LogLevel::ERR:
    return spdlog::level::err;
  case polycube::LogLevel::CRITICAL:
    return spdlog::level::critical;
  case polycube::LogLevel::OFF:
    return spdlog::level::off;
  }
}

size_t get_possible_cpu_count() {
  return ebpf::BPFTable::get_possible_cpu_count();
}

std::string cube_type_to_string(CubeType type) {
  switch (type) {
    case CubeType::TC:
      return "TC";
    case CubeType::XDP_DRV:
      return "XDP_DRV";
    case CubeType::XDP_SKB:
      return "XDP_SKB";
  }
}

CubeType string_to_cube_type(const std::string &type_str) {
  if (iequals(type_str, "TC")) {
    return CubeType::TC;
  } else if (iequals(type_str, "XDP_DRV")) {
    return CubeType::XDP_DRV;
  } else if (iequals(type_str, "XDP_SKB")) {
    return CubeType::XDP_SKB;
  }
}

std::string port_status_to_string(PortStatus status) {
  switch (status) {
  case PortStatus::UP:
    return "UP";
  case PortStatus::DOWN:
    return "DOWN";
  }
}

PortStatus string_to_port_status(const std::string &status) {
  if (iequals(status, "UP")) {
    return PortStatus::UP;
  } else if (iequals(status, "DOWN")) {
    return PortStatus::DOWN;
  }
}

}  // namespace polycube
