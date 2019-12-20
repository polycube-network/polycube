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

#include <libyang/libyang.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../../Types/Scalar.h"

namespace polycube::polycubed {
class PolycubedCore;
}

namespace polycube::polycubed::Rest::Validators {
class ValueValidator;
}

namespace polycube::polycubed::Rest::Resources::Body {
class CaseResource;
class ChoiceResource;
class JsonValueField;
class JsonNodeField;
class LeafResource;
class LeafListResource;
class ListResource;
class ListKey;
class ParentResource;
class Service;

class AbstractFactory {
 public:
  std::unique_ptr<Body::JsonValueField> JsonValueField() const;

  std::unique_ptr<Body::JsonValueField> JsonValueField(
      LY_DATA_TYPE type,
      std::vector<std::shared_ptr<Validators::ValueValidator>> &&validators)
      const;

  std::unique_ptr<CaseResource> BodyCase(
      const std::string &name, const std::string &description,
      const std::string &cli_example, const ParentResource *const parent) const;

  std::unique_ptr<ChoiceResource> BodyChoice(
      const std::string &name, const std::string &description,
      const std::string &cli_example, const ParentResource *const parent,
      bool mandatory, std::unique_ptr<const std::string> &&default_case) const;

  std::unique_ptr<LeafResource> BodyLeaf(
      const std::string &name, const std::string &description,
      const std::string &cli_example, const ParentResource *const parent,
      std::unique_ptr<Body::JsonValueField> &&value_field,
      const std::vector<Body::JsonNodeField> &node_fields, bool configuration,
      bool init_only_config, bool mandatory, Types::Scalar type,
      std::unique_ptr<const std::string> &&default_value) const;

  std::unique_ptr<LeafListResource> BodyLeafList(
      const std::string &name, const std::string &description,
      const std::string &cli_example, const ParentResource *const parent,
      std::unique_ptr<Body::JsonValueField> &&value_field,
      const std::vector<JsonNodeField> &node_fields, bool configuration,
      bool init_only_config, bool mandatory, Types::Scalar type,
      std::vector<std::string> &&default_value) const;

  std::unique_ptr<ListResource> BodyList(
      const std::string &name, const std::string &description,
      const std::string &cli_example, const ParentResource *const parent,
      std::vector<Resources::Body::ListKey> &&keys,
      const std::vector<JsonNodeField> &node_fields, bool configuration,
      bool init_only_config) const;

  std::unique_ptr<ParentResource> BodyGeneric(
      const std::string &name, const std::string &description,
      const std::string &cli_example, const ParentResource *const parent,
      const std::vector<JsonNodeField> &node_fields, bool configuration,
      bool init_only_config, bool container_presence) const;

 protected:
  PolycubedCore *const core_;
  explicit AbstractFactory(PolycubedCore *core);
};
}  // namespace polycube::polycubed::Rest::Resources::Body
