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

#include "server/Resources/Endpoint/Service.h"

struct InfoMetric {
  const std::string nameMetric; //the name of the metric the will be scraped
  const std::string typeMetric; //GAUGE, COUNTER
  const std::string pathMetric; //tha json path to take the value from the json of a cube
  const std::string helpMetric;
  const std::string typeOperation;  
  std::double_t value;
};
//about type-operation, maybe there is a better way: 
//FILTER: after a filter query we need the length but 
//we cannot do that so we use size() on the result (like the number of)

struct ServiceMetadata {
  std::string description;
  std::string version;
  std::string pyangGitRepoId;
  std::string swaggerCodegenGitRepoId;
  std::string dataModel;
  std::string requiredKernelVersion;
  std::vector<InfoMetric> Metrics;
};

namespace polycube::polycubed {
class ManagementInterface {
 public:
  inline const std::shared_ptr<polycubed::Rest::Resources::Endpoint::Service>
  get_service() {
    return service_;
  }

 protected:
  virtual ~ManagementInterface() {
    if (service_) {
      service_->ClearCubes();
    }
  };
  std::shared_ptr<polycubed::Rest::Resources::Endpoint::Service> service_;
};
}  // namespace polycube::polycubed
