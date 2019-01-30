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

#include "../Body/CaseResource.h"
#include "ParentResource.h"

namespace polycube::polycubed::Rest::Resources::Endpoint {
class CaseResource : public ParentResource, public Body::CaseResource {
 public:
  CaseResource(const std::string &name, const std::string &description,
               const std::string &cli_example,
               const Body::ParentResource *const parent, PolycubedCore *core);

  ~CaseResource() override = default;

  Response Help(const std::string &cube_name, HelpType type,
                const ListKeyValues &keys) final;
};
}  // namespace polycube::polycubed::Rest::Resources::Endpoint
