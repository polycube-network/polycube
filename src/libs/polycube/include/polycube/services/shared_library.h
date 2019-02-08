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

/*
 * shared_library.h implements ManagementInterface.  This file should be
 * included in a .cpp file and compiled as a shared library.
 * A service should define the MANAGER_TYPE macro to the type of the Manager
 * it intends to use
 */

#pragma once

#include "cube_factory.h"
#include "management_interface.h"

namespace polycube {
namespace service {

static ManagementInterface *mgmt;

#ifndef MANAGER_TYPE
#error "MANAGER_TYPE not defined"
#endif

#ifndef SERVICE_DESCRIPTION
#define SERVICE_DESCRIPTION ""
#endif

#ifndef SERVICE_VERSION
#define SERVICE_VERSION ""
#endif

#ifndef SERVICE_PYANG_GIT
#define SERVICE_PYANG_GIT ""
#endif

#ifndef SERVICE_SWAGGER_CODEGEN_GIT
#define SERVICE_SWAGGER_CODEGEN_GIT ""
#endif

#ifndef SERVICE_REQUIRED_KERNEL_VERSION
#define SERVICE_REQUIRED_KERNEL_VERSION ""
#endif

// TODO: I am sure that there is a best way to pass this pointer around
CubeFactory *factory_;
std::string logfile_;

extern "C" ServiceMetadata init(CubeFactory *factory, std::string logfile) {
  ServiceMetadata metadata;
  factory_ = factory;
  logfile_ = logfile;
  mgmt = new MANAGER_TYPE();

  metadata.description = SERVICE_DESCRIPTION;
  metadata.version = SERVICE_VERSION;
  metadata.pyangGitRepoId = SERVICE_PYANG_GIT;
  metadata.swaggerCodegenGitRepoId = SERVICE_SWAGGER_CODEGEN_GIT;
  metadata.dataModel = std::string(SERVICE_DATA_MODEL);
  metadata.requiredKernelVersion = std::string(SERVICE_REQUIRED_KERNEL_VERSION);
  return metadata;
}

extern "C" void control_handler(const HttpHandleRequest &request,
                                HttpHandleResponse &response) {
  if (mgmt == nullptr) {
    return;
  }
  mgmt->control_handler(request, response);
}

extern "C" void uninit() {
  if (mgmt == nullptr) {
    return;
  }
  delete mgmt;
}

}  // namespace service
}  // namespace polycube
