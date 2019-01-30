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

#include "server/Parser/Yang.h"
#include "server/Resources/Data/Lib/ConcreteFactory.h"
#include "server/Resources/Data/Lib/Service.h"

#include "polycube/services/cube_factory.h"

namespace polycube::polycubed {

ManagementLib::ManagementLib(const std::string &uri,
                             const std::string &base_url,
                             const std::string &name, PolycubedCore *core)
    : uri_(uri), base_url_(base_url), name_(name), core_(core) {}

void ManagementLib::load() {
  auto factory = std::make_unique<Rest::Resources::Data::Lib::ConcreteFactory>(
      uri_, core_);
  auto parser = Rest::Parser::Yang(std::move(factory));
  service_ = parser.Parse(base_url_, name_);
}

ServiceMetadata ManagementLib::init(service::CubeFactory *factory,
                                    std::string logfile) {
  return std::dynamic_pointer_cast<Rest::Resources::Data::Lib::Service>(
             service_)
      ->init(factory, logfile);
}
}  // namespace polycube::polycubed
