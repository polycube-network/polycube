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
#include "Resource.h"

#include <string>
#include <fstream>
#include <rest_server.h>
#include "../../config.h"

namespace polycube::polycubed::Rest::Resources::Endpoint {

Resource::Resource(const std::string &rest_endpoint)
    : rest_endpoint_{rest_endpoint} {}

Operation Resource::OperationType(bool update, bool initialization) {
  if (!update) {
    return Operation::kCreate;
  } else {
    if (initialization) {
      return Operation::kReplace;
    } else {
      return Operation::kUpdate;
    }
  }
}

bool Resource::isOperationSuccessful(ErrorTag errorTag) {
  return errorTag == ErrorTag::kOk ||
         errorTag == ErrorTag::kCreated ||
         errorTag == ErrorTag::kNoContent;
}

}  // namespace polycube::polycubed::Rest::Resources::Endpoint
