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
#include "CaseResource.h"

#include <memory>
#include <string>
#include <utility>

#include "JsonNodeField.h"

namespace polycube::polycubed::Rest::Resources::Body {
CaseResource::CaseResource(const std::string &name,
                           const std::string &description,
                           const std::string &cli_example,
                           const Body::ParentResource *parent,
                           PolycubedCore *core)
    : ParentResource(name, description, cli_example, parent, core,
                     std::vector<JsonNodeField>{}, false, false, false) {}
}  // namespace polycube::polycubed::Rest::Resources::Body
