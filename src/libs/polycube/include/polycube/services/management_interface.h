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

#include <map>
#include <string>

#include "http.h"

namespace polycube {
namespace service {

struct ServiceMetadata {
  std::string description;
  std::string version;
  std::string pyangGitRepoId;
  std::string swaggerCodegenGitRepoId;
  std::string dataModel;
  std::string requiredKernelVersion;
  // Add here additional service metadata
};

class ManagementInterface {
 public:
  virtual ~ManagementInterface(){};
  virtual void control_handler(const HttpHandleRequest &request,
                               HttpHandleResponse &response) = 0;
};

}  // namespace service
}  // namespace polycube
