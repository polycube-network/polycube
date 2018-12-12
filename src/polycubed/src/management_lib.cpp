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

#include "management_lib.h"

namespace polycube {
namespace polycubed {

ManagementLib::ManagementLib() {}
ManagementLib::ManagementLib(const std::string &libName) : library(libName) {}

ManagementLib::~ManagementLib() {
  unload();
}

void ManagementLib::setLibName(const std::string &libName) {
  library.setLibName(libName);
}

bool ManagementLib::load() {
  library.load();
  loadFunctions();
  return loaded();
}

bool ManagementLib::loaded() const {
  if (library.loaded())
    if (isloaded)
      return true;
  return false;
}

void ManagementLib::unload() {
  if (!loaded())
    return;
  uninit_f();
  library.unload();
  unloadFunctions();
}

void ManagementLib::unloadFunctions() {
  control_handler_f = nullptr;
  isloaded = false;
}

bool ManagementLib::loadFunctions() {
  isloaded = true;

  init_f = (init_t)library.loadFunction("init");
  if (init_f == NULL)
    isloaded = false;

  control_handler_f = (control_handler_t)library.loadFunction("control_handler");
  if (control_handler_f == NULL)
    isloaded = false;

  uninit_f = (uninit_t)library.loadFunction("uninit");
  if (uninit_f == NULL)
    isloaded = false;

  return isloaded;
}

ServiceMetadata ManagementLib::init(CubeFactory *factory, std::string logfile) {
  ServiceMetadata service_md;
  if (!loaded())
    throw std::runtime_error("library not loaded");
  service_md = init_f(factory, logfile);

  return service_md;
}

void ManagementLib::control_handler(const HttpHandleRequest &request,
                                HttpHandleResponse &response) {
  if (!loaded())
    throw std::runtime_error("library not loaded");
  control_handler_f(request, response);
}

}  // namespace polycubed
}  // namespace polycube
