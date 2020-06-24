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

#include "spdlog/spdlog.h"
#include <string>

namespace polycube {

namespace service {
  enum class CubeType;
  enum class PortStatus;
}

using polycube::service::CubeType;
using polycube::service::PortStatus;

// Taken from
// https://github.com/gabime/spdlog/blob/master/include/spdlog/common.h

enum class LogLevel {
  TRACE = 0,
  DEBUG = 1,
  INFO = 2,
  WARN = 3,
  ERR = 4,
  CRITICAL = 5,
  OFF = 6
};

// https://stackoverflow.com/a/4119881
bool iequals(const std::string &a, const std::string &b);

std::string logLevelString(polycube::LogLevel l);
polycube::LogLevel stringLogLevel(const std::string &level);
/* Log level representation in polycube and spdlog library could change,
 * so in order to avoid conflicts with function perform the conversion
 * between the two representations
 */
spdlog::level::level_enum logLevelToSPDLog(polycube::LogLevel level);

std::string cube_type_to_string(CubeType type);
CubeType string_to_cube_type(const std::string &type_str);

std::string port_status_to_string(PortStatus status);
PortStatus string_to_port_status(const std::string &status);

size_t get_possible_cpu_count();

}  // namespace polycube
