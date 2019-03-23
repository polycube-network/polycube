/*
 * Copyright 2019 The Polycube Authors
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

#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "../AbstractFactory.h"
#include "polycube/services/shared_lib_elements.h"

namespace polycube::polycubed::Rest::Resources::Data::BaseModel {

class ConcreteFactory : public Data::AbstractFactory {
 public:
  explicit ConcreteFactory(const std::string &file_name, PolycubedCore *core);
  bool IsBaseModel(const std::queue<std::string> &tree_names) const;

  std::unique_ptr<Endpoint::CaseResource> RestCase(
      const std::queue<std::string> &tree_names, const std::string &name,
      const std::string &description, const std::string &cli_example,
      const Body::ParentResource *parent) const final;

  std::unique_ptr<Endpoint::ChoiceResource> RestChoice(
      const std::queue<std::string> &tree_names, const std::string &name,
      const std::string &description, const std::string &cli_example,
      const Body::ParentResource *parent, bool mandatory,
      std::unique_ptr<const std::string> &&default_case) const final;

  std::unique_ptr<Endpoint::LeafResource> RestLeaf(
      const std::queue<std::string> &tree_names, const std::string &name,
      const std::string &description, const std::string &cli_example,
      const std::string &rest_endpoint, const Body::ParentResource *parent,
      std::unique_ptr<Body::JsonValueField> &&value_field,
      const std::vector<Body::JsonNodeField> &node_fields, bool configuration,
      bool init_only_config, bool mandatory, Types::Scalar type,
      std::unique_ptr<const std::string> &&default_value,
      bool is_enum, const std::vector<std::string> &values) const final;

  std::unique_ptr<Endpoint::LeafListResource> RestLeafList(
      const std::queue<std::string> &tree_names, const std::string &name,
      const std::string &description, const std::string &cli_example,
      const std::string &rest_endpoint, const Body::ParentResource *parent,
      std::unique_ptr<Body::JsonValueField> &&value_field,
      const std::vector<Body::JsonNodeField> &node_fields, bool configuration,
      bool init_only_config, bool mandatory, Types::Scalar type,
      std::vector<std::string> &&default_value) const final;

  std::unique_ptr<Endpoint::ListResource> RestList(
      const std::queue<std::string> &tree_names, const std::string &name,
      const std::string &description, const std::string &cli_example,
      const std::string &rest_endpoint,
      const std::string &rest_endpoint_whole_list,
      const Body::ParentResource *parent, bool configuration,
      bool init_only_config, std::vector<Resources::Body::ListKey> &&keys,
      const std::vector<Body::JsonNodeField> &node_fields) const final;

  std::unique_ptr<Endpoint::ParentResource> RestGeneric(
      const std::queue<std::string> &tree_names, const std::string &name,
      const std::string &description, const std::string &cli_example,
      const std::string &rest_endpoint, const Body::ParentResource *parent,
      const std::vector<Body::JsonNodeField> &node_fields, bool configuration,
      bool init_only_config, bool container_presence,
      bool rpc_action) const final;

  std::unique_ptr<Endpoint::Service> RestService(
      const std::queue<std::string> &tree_names, const std::string &name,
      const std::string &description, const std::string &cli_example,
      std::string base_endpoint, std::string version) const final;

 private:
  PolycubedCore *core_;
};
}  // namespace polycube::polycubed::Rest::Resources::Data::BaseModel
