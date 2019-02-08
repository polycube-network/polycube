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

#include "dynamic_library.h"
#include "polycube/services/cube_factory.h"
#include "polycube/services/management_interface.h"

namespace polycube {
namespace polycubed {

using service::CubeFactory;
using service::ManagementInterface;
using service::HttpHandleRequest;
using service::HttpHandleResponse;
using service::ServiceMetadata;

class ManagementLib : public ManagementInterface {
 public:
  ManagementLib();
  ManagementLib(const std::string &libName);
  ~ManagementLib();

  void setLibName(const std::string &libName);
  bool load();
  void unload();
  bool loaded() const;

  ServiceMetadata init(CubeFactory *factory, std::string logfile);
  void control_handler(const HttpHandleRequest &request,
                       HttpHandleResponse &response) override;

  typedef ServiceMetadata (*init_t)(CubeFactory *polycubed,
                                    std::string logfile);
  typedef void (*control_handler_t)(const HttpHandleRequest &request,
                                    HttpHandleResponse &response);
  typedef void (*uninit_t)();

 private:
  DynamicLibrary library;
  bool loadFunctions();
  void unloadFunctions();
  bool isloaded = false;

  init_t init_f;
  control_handler_t control_handler_f;
  uninit_t uninit_f;
};

}  // namespace polycubed
}  // namespace polycube
