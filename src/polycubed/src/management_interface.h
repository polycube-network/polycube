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

#include <memory>

#include "polycube/services/management_interface.h"
#include "server/Resources/Endpoint/Service.h"

namespace polycube::polycubed {
class ManagementInterface : public service::ManagementInterface {
 public:
  inline const std::shared_ptr<polycubed::Rest::Resources::Endpoint::Service>
  get_service() {
    return service_;
  }

 protected:
  ~ManagementInterface() override {
    service_->ClearCubes();
  };
  std::shared_ptr<polycubed::Rest::Resources::Endpoint::Service> service_;
};
}  // namespace polycube::polycubed
