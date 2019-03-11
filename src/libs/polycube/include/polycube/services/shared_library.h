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
 * shared_library.h This file should be included in a .cpp file and compiled
 * as a shared library.
 */
#pragma once

#include "cube_factory.h"

namespace polycube {
namespace service {

#ifndef SERVICE_PYANG_GIT
#define SERVICE_PYANG_GIT ""
#endif

#ifndef SERVICE_SWAGGER_CODEGEN_GIT
#define SERVICE_SWAGGER_CODEGEN_GIT ""
#endif

// TODO: I am sure that there is a best way to pass this pointer around
CubeFactory *factory_;
std::string logfile_;

extern "C" void init(CubeFactory *factory, const char *logfile) {
  factory_ = factory;
  logfile_ = std::string{logfile};
}

extern "C" const char *pyang_git() {
  return SERVICE_PYANG_GIT;
}

extern "C" const char *swagger_codegen_git() {
  return SERVICE_SWAGGER_CODEGEN_GIT;
}

}  // namespace service
}  // namespace polycube
