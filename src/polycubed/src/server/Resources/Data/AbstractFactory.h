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
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "../Body/AbstractFactory.h"

namespace polycube::polycubed::Rest::Resources::Endpoint {
class CaseResource;
class ChoiceResource;
class LeafResource;
class LeafListResource;
class ListResource;
class ParentResource;
class Service;
}  // namespace polycube::polycubed::Rest::Resources::Endpoint

namespace polycube::polycubed::Rest::Resources::Data {

class AbstractFactory : public Body::AbstractFactory {
 public:
  virtual std::unique_ptr<Endpoint::CaseResource> RestCase(
      const std::queue<std::string> &tree_names, const std::string &name,
      const std::string &description, const std::string &cli_example,
      const Body::ParentResource *parent) const = 0;

  virtual std::unique_ptr<Endpoint::ChoiceResource> RestChoice(
      const std::queue<std::string> &tree_names, const std::string &name,
      const std::string &description, const std::string &cli_example,
      const Body::ParentResource *parent, bool mandatory,
      std::unique_ptr<const std::string> &&default_case) const = 0;

  virtual std::unique_ptr<Endpoint::LeafResource> RestLeaf(
      const std::queue<std::string> &tree_names, const std::string &name,
      const std::string &description, const std::string &cli_example,
      const std::string &rest_endpoint, const Body::ParentResource *parent,
      std::unique_ptr<Body::JsonValueField> &&value_field,
      const std::vector<Body::JsonNodeField> &node_fields, bool configuration,
      bool init_only_config, bool mandatory, Types::Scalar type,
      std::unique_ptr<const std::string> &&default_value) const = 0;

  virtual std::unique_ptr<Endpoint::LeafListResource> RestLeafList(
      const std::queue<std::string> &tree_names, const std::string &name,
      const std::string &description, const std::string &cli_example,
      const std::string &rest_endpoint, const Body::ParentResource *parent,
      std::unique_ptr<Body::JsonValueField> &&value_field,
      const std::vector<Body::JsonNodeField> &node_fields, bool configuration,
      bool init_only_config, bool mandatory, Types::Scalar type,
      std::vector<std::string> &&default_value) const = 0;

  virtual std::unique_ptr<Endpoint::ListResource> RestList(
      const std::queue<std::string> &tree_names, const std::string &name,
      const std::string &description, const std::string &cli_example,
      const std::string &rest_endpoint,
      const std::string &rest_endpoint_whole_list,
      const Body::ParentResource *parent, bool configuration,
      bool init_only_config, std::vector<Resources::Body::ListKey> &&keys,
      const std::vector<Body::JsonNodeField> &node_fields) const = 0;

  virtual std::unique_ptr<Endpoint::ParentResource> RestGeneric(
      const std::queue<std::string> &tree_names, const std::string &name,
      const std::string &description, const std::string &cli_example,
      const std::string &rest_endpoint, const Body::ParentResource *parent,
      const std::vector<Body::JsonNodeField> &node_fields, bool configuration,
      bool init_only_config, bool container_presence,
      bool rpc_action) const = 0;

  virtual std::unique_ptr<Endpoint::Service> RestService(
      const std::queue<std::string> &tree_names, const std::string &name,
      const std::string &description, const std::string &cli_example,
      std::string base_endpoint, std::string version) const = 0;

 protected:
  explicit AbstractFactory(PolycubedCore *core) : Body::AbstractFactory(core) {}
};
}  // namespace polycube::polycubed::Rest::Resources::Data
