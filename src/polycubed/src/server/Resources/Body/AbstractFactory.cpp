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
#include "AbstractFactory.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "CaseResource.h"
#include "ChoiceResource.h"
#include "JsonNodeField.h"
#include "JsonValueField.h"
#include "LeafListResource.h"
#include "LeafResource.h"
#include "ListKey.h"
#include "ListResource.h"
#include "ParentResource.h"

namespace polycube::polycubed::Rest::Resources::Body {
std::unique_ptr<Body::JsonValueField> AbstractFactory::JsonValueField() const {
  return std::make_unique<Body::JsonValueField>();
}

std::unique_ptr<Body::JsonValueField> AbstractFactory::JsonValueField(
    LY_DATA_TYPE type,
    std::vector<std::shared_ptr<Validators::ValueValidator>> &&validators)
    const {
  return std::make_unique<Body::JsonValueField>(type, std::move(validators));
}

std::unique_ptr<CaseResource> AbstractFactory::BodyCase(
    const std::string &name, const std::string &description,
    const std::string &cli_example, const Body::ParentResource *parent) const {
  return std::make_unique<CaseResource>(name, description, cli_example, parent,
                                        core_);
}

std::unique_ptr<ChoiceResource> AbstractFactory::BodyChoice(
    const std::string &name, const std::string &description,
    const std::string &cli_example, const Body::ParentResource *parent,
    bool mandatory, std::unique_ptr<const std::string> &&default_case) const {
  return std::make_unique<ChoiceResource>(name, description, cli_example,
                                          parent, core_, mandatory,
                                          std::move(default_case));
}

std::unique_ptr<LeafResource> AbstractFactory::BodyLeaf(
    const std::string &name, const std::string &description,
    const std::string &cli_example, const Body::ParentResource *parent,
    std::unique_ptr<Body::JsonValueField> &&value_field,
    const std::vector<Body::JsonNodeField> &node_fields, bool configuration,
    bool init_only_config, bool mandatory, Types::Scalar type,
    std::unique_ptr<const std::string> &&default_value) const {
  return std::make_unique<LeafResource>(
      name, description, cli_example, parent, core_, std::move(value_field),
      node_fields, configuration, init_only_config, mandatory, type,
      std::move(default_value));
}

std::unique_ptr<LeafListResource> AbstractFactory::BodyLeafList(
    const std::string &name, const std::string &description,
    const std::string &cli_example, const Body::ParentResource *parent,
    std::unique_ptr<Body::JsonValueField> &&value_field,
    const std::vector<JsonNodeField> &node_fields, bool configuration,
    bool init_only_config, bool mandatory, Types::Scalar type,
    std::vector<std::string> &&default_value) const {
  return std::make_unique<LeafListResource>(
      name, description, cli_example, parent, core_, std::move(value_field),
      node_fields, configuration, init_only_config, mandatory, type,
      std::move(default_value));
}

std::unique_ptr<ListResource> AbstractFactory::BodyList(
    const std::string &name, const std::string &description,
    const std::string &cli_example, const Body::ParentResource *parent,
    std::vector<Resources::Body::ListKey> &&keys,
    const std::vector<JsonNodeField> &node_fields, bool configuration,
    bool init_only_config) const {
  return std::make_unique<ListResource>(name, description, cli_example, parent,
                                        core_, std::move(keys), node_fields,
                                        configuration, init_only_config);
}

std::unique_ptr<ParentResource> AbstractFactory::BodyGeneric(
    const std::string &name, const std::string &description,
    const std::string &cli_example, const std::string &nameMetric, const Body::ParentResource *parent,
    const std::vector<JsonNodeField> &node_fields, bool configuration,
    bool init_only_config, bool container_presence) const {
  return std::make_unique<ParentResource>(
      name, description, cli_example, parent, core_, node_fields, configuration,
      init_only_config, container_presence);
}

AbstractFactory::AbstractFactory(PolycubedCore *core) : core_(core) {}
}  // namespace polycube::polycubed::Rest::Resources::Body
