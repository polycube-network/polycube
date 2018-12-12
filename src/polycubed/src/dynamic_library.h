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

#include <dlfcn.h>
#include <iostream>
#include <string>

#include <spdlog/spdlog.h>

namespace polycube {
namespace polycubed {

class DynamicLibrary {
 public:
  DynamicLibrary();
  DynamicLibrary(const std::string &lib_name);
  ~DynamicLibrary();

  void setLibName(const std::string &lib_name);
  bool load();
  void unload();
  bool loaded() const;
  void *loadFunction(std::string funcName) const;

 private:
  std::string lib_name_;
  void *lib_;
  void *loadLibrary();
};

}  // namespace polycubed
}  // namespace polycube
