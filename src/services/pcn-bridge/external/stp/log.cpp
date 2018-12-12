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

#include "log.h"

#include <memory>
#include <stdarg.h>
#include <stdio.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>

static std::shared_ptr<spdlog::logger> l = spdlog::get("pcn-bridge") == nullptr
        ? spdlog::stdout_logger_mt("pcn-bridge")
        : spdlog::get("pcn-bridge");

/********** log.c **********/
void VLOG_WARN(const char *format, ...) {
  char buf[100];
  va_list arglist;
  va_start(arglist, format);
  vsnprintf(buf, sizeof(buf), format, arglist);
  va_end(arglist);
  l->warn(buf);
}

void VLOG_DBG(const char *format, ...) {
  char buf[100];
  va_list arglist;
  va_start(arglist, format);
  vsnprintf(buf, sizeof(buf), format, arglist);
  va_end(arglist);
  l->debug(buf);
}

void VLOG_INFO(const char *format, ...) {
  char buf[100];
  va_list arglist;
  va_start(arglist, format);
  vsnprintf(buf, sizeof(buf), format, arglist);
  va_end(arglist);
  l->info(buf);
}
