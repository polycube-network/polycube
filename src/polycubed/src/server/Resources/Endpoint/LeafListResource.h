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
#include <string>
#include <vector>

#include "../Body/JsonValueField.h"
#include "../Body/LeafListResource.h"
#include "LeafResource.h"

namespace polycube::polycubed::Rest::Resources::Endpoint {
using Pistache::Http::ResponseWriter;
using Pistache::Rest::Request;

class LeafListResource : public LeafResource, public Body::LeafListResource {
 public:
  LeafListResource(const std::string &name, const std::string &description,
                   const std::string &cli_example,
                   const std::string &rest_endpoint,
                   const Body::ParentResource *const parent,
                   PolycubedCore *core,
                   std::unique_ptr<Body::JsonValueField> &&value_field,
                   const std::vector<Body::JsonNodeField> &node_fields,
                   bool configuration, bool init_only_config, bool mandatory,
                   Types::Scalar type,
                   std::vector<std::string> &&default_value);

  ~LeafListResource() override;

  virtual std::shared_ptr<LeafResource> &Entry(
      const std::string &value) const = 0;

 private:
  void get_entry(const Request &request, ResponseWriter response);
};
}  // namespace polycube::polycubed::Rest::Resources::Endpoint
