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

#include "dynamic_library.h"

namespace polycube {
namespace polycubed {

DynamicLibrary::DynamicLibrary() : lib_name_(""), lib_(nullptr) {}
DynamicLibrary::DynamicLibrary(const std::string &lib_name)
    : lib_name_(lib_name), lib_(nullptr) {}

DynamicLibrary::~DynamicLibrary() {
  unload();
}

void DynamicLibrary::setLibName(const std::string &lib_name) {
  unload();
  lib_name_ = lib_name;
}

bool DynamicLibrary::load() {
  spdlog::get("polycubed")->trace("dynamic library load");
  unload();
  lib_ = loadLibrary();
  return loaded();
}

void *DynamicLibrary::loadLibrary() {
  spdlog::get("polycubed")->trace("loading {0}", lib_name_.c_str());
  void *lib_ = dlopen(lib_name_.c_str(), RTLD_LAZY);
  char *error = dlerror();
  if (error != nullptr) {
    std::cerr << error << std::endl;
  }
  return lib_;
}

bool DynamicLibrary::loaded() const {
  return lib_ != nullptr;
}

void DynamicLibrary::unload() {
  if (lib_ == nullptr)
    return;
  dlclose(lib_);
  spdlog::get("polycubed")->trace("unloading {0}", lib_name_);
  lib_ = nullptr;
}

void *DynamicLibrary::loadFunction(std::string funcName) const {
  spdlog::get("polycubed")->trace("load function {0}", funcName.c_str());
  void *func = dlsym(lib_, funcName.c_str());
  if (func == nullptr)
    std::cerr << dlerror() << std::endl;
  return func;
}

}  // namespace polycubed
}  // namespace polycube
