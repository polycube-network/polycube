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

#include "../../Validators/InSetValidator.h"
#include "../Body/ChoiceResource.h"
#include "ParentResource.h"

namespace polycube::polycubed::Rest::Resources::Endpoint {
using Pistache::Http::ResponseWriter;
using Pistache::Rest::Request;

class ChoiceResource : public ParentResource, public Body::ChoiceResource {
 public:
  ChoiceResource(const std::string &name, const std::string &description,
                 const std::string &cli_example,
                 const Body::ParentResource *const parent, PolycubedCore *core,
                 const std::vector<Body::JsonNodeField> &node_fields,
                 bool mandatory,
                 std::unique_ptr<const std::string> &&default_case);

  void AddChild(std::shared_ptr<Body::Resource> &&child) final;

  using Body::ChoiceResource::IsMandatory;
  using Body::ChoiceResource::SetDefaultIfMissing;

 private:
  Validators::InSetValidator choice_;
};
}  // namespace polycube::polycubed::Rest::Resources::Endpoint
