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

#include <string>
#include <spdlog/spdlog.h>

namespace polycube {

// Taken from https://github.com/gabime/spdlog/blob/master/include/spdlog/common.h

enum class LogLevel {
  TRACE = 0,
  DEBUG = 1,
  INFO = 2,
  WARN = 3,
  ERR = 4,
  CRITICAL = 5,
  OFF = 6
};

std::string logLevelString(polycube::LogLevel l);
polycube::LogLevel stringLogLevel(const std::string &level);
/* Log level represantation in polycube and spdlog library could change,
 * so in order to avoid conflicts whis function perform the conversion
 * between the two representations
 */
spdlog::level::level_enum logLevelToSPDLog(polycube::LogLevel level);

size_t get_possible_cpu_count();

}  // namespace polycube
