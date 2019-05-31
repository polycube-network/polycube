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
#include "ConcreteFactory.h"

#include <memory>
#include <string>
#include <vector>

#include "../../Body/JsonNodeField.h"

#include "../../Endpoint/LeafResource.h"
#include "../../Endpoint/ListResource.h"
#include "../../Endpoint/ParentResource.h"
#include "../../Endpoint/Service.h"

#include "LeafResource.h"

#include "polycubed_core.h"

namespace polycube::polycubed::Rest::Resources::Data::BaseModel {

ConcreteFactory::ConcreteFactory(const std::string &file_name,
                                 PolycubedCore *core)
    : AbstractFactory(core), core_(core) {}

bool ConcreteFactory::IsBaseModel(
    const std::queue<std::string> &tree_names) const {
  // TODO: what to check  here? we don't need
  auto tree_names_ = tree_names;
  tree_names_.pop();

  if (tree_names_.size() == 1) {
    auto leaf = tree_names_.front();
    if (leaf == "type" || leaf == "uuid" || leaf == "loglevel" ||
        leaf == "parent" || leaf == "service-name" || leaf == "shadow" || leaf == "span") {
      return true;
    }
  } else if (tree_names_.size() == 2) {
    if (tree_names_.front() != "ports") {
      return false;
    }

    tree_names_.pop();
    auto leaf = tree_names_.front();

    if (leaf == "uuid" || leaf == "status" || leaf == "peer") {
      return true;
    }
  }

  return false;
}

std::unique_ptr<Endpoint::CaseResource> ConcreteFactory::RestCase(
    const std::queue<std::string> &tree_names, const std::string &name,
    const std::string &description, const std::string &cli_example,
    const Body::ParentResource *parent) const {
  throw std::invalid_argument(
      "Yang case node not supported with shared object protocol.");
}

std::unique_ptr<Endpoint::ChoiceResource> ConcreteFactory::RestChoice(
    const std::queue<std::string> &tree_names, const std::string &name,
    const std::string &description, const std::string &cli_example,
    const Body::ParentResource *parent, bool mandatory,
    std::unique_ptr<const std::string> &&default_case) const {
  throw std::invalid_argument(
      "Yang choice node not supported with shared object protocol.");
}

/*
 * This function creates a set of LeafResource handlers for the leafs present
 * on the base datamodel.
 * So far those are:
 * polycube-base:
 * - uuid
 * - type
 * - loglevel
 * - parent
 * polycyube-standard:
 * - ports/name
 * - ports/uuid
 * - ports/status
 * - ports/peer
 *
 * tree_names is used to understand what resource should be created, if
 * there is unknown resource the function throws an error.
 */
std::unique_ptr<Endpoint::LeafResource> ConcreteFactory::RestLeaf(
    const std::queue<std::string> &tree_names, const std::string &name,
    const std::string &description, const std::string &cli_example,
    const std::string &rest_endpoint, const Body::ParentResource *parent,
    std::unique_ptr<Body::JsonValueField> &&value_field,
    const std::vector<Body::JsonNodeField> &node_fields, bool configuration,
    bool init_only_config, bool mandatory, Types::Scalar type,
    std::unique_ptr<const std::string> &&default_value,
    bool is_enum, const std::vector<std::string> &values) const {
  // TODO: what to check  here? we don't need
  auto tree_names_ = tree_names;
  tree_names_.pop();

  std::function<Response(const std::string &, const ListKeyValues &keys)>
      read_handler_;
  std::function<Response(const std::string &, const nlohmann::json &,
                         const ListKeyValues &, Endpoint::Operation)>
      replace_handler_;

  // TODO: I need to capture this variable inside the lambda functions,
  // capturing "this" for me is not working
  auto local_core = this->core_;

  if (tree_names_.size() == 1) {
    auto leaf = tree_names_.front();
    if (leaf == "type") {
      read_handler_ = [local_core](const std::string &cube_name,
                                   const ListKeyValues &keys) -> Response {
        return local_core->base_model()->get_type(cube_name);
      };
    } else if (leaf == "uuid") {
      read_handler_ = [local_core](const std::string &cube_name,
                                   const ListKeyValues &keys) -> Response {
        return local_core->base_model()->get_uuid(cube_name);
      };
    } else if (leaf == "loglevel") {
      read_handler_ = [local_core](const std::string &cube_name,
                                   const ListKeyValues &keys) -> Response {
        return local_core->base_model()->get_loglevel(cube_name);
      };

      replace_handler_ = [local_core](
          const std::string &cube_name, const nlohmann::json &json,
          const ListKeyValues &keys, Endpoint::Operation op) -> Response {
        return local_core->base_model()->set_loglevel(cube_name, json);
      };
    } else if (leaf == "parent") {
      // parent for transparent services
      read_handler_ = [local_core](const std::string &cube_name,
                                   const ListKeyValues &keys) -> Response {
        return local_core->base_model()->get_parent(cube_name);
      };
    } else if (leaf == "service-name") {
      read_handler_ = [local_core](const std::string &cube_name,
                                   const ListKeyValues &keys) -> Response {
        return local_core->base_model()->get_service(cube_name);
      };
    } else if (leaf == "shadow") {
      read_handler_ = [local_core](const std::string &cube_name,
                                   const ListKeyValues &keys) -> Response {
        return local_core->base_model()->get_shadow(cube_name);
      };
    } else if (leaf == "span") {
      read_handler_ = [local_core](const std::string &cube_name,
                                   const ListKeyValues &keys) -> Response {
        return local_core->base_model()->get_span(cube_name);
      };

      replace_handler_ = [local_core](
          const std::string &cube_name, const nlohmann::json &json,
          const ListKeyValues &keys, Endpoint::Operation op) -> Response {
        return local_core->base_model()->set_span(cube_name, json);
      };
    } else {
      throw std::runtime_error("unkown element found in base datamodel:" + leaf);
    }
  } else if (tree_names_.size() == 2) {
    if (tree_names_.front() != "ports") {
      throw std::runtime_error("unkown element found in base datamodel");
    }

    tree_names_.pop();
    auto leaf = tree_names_.front();
    if (leaf == "uuid") {
      read_handler_ = [local_core](const std::string &cube_name,
                                   const ListKeyValues &keys) -> Response {
        return local_core->base_model()->get_port_uuid(cube_name,
                                                       keys[0].value);
      };
    } else if (leaf == "status") {
      read_handler_ = [local_core](const std::string &cube_name,
                                   const ListKeyValues &keys) -> Response {
        return local_core->base_model()->get_port_status(cube_name,
                                                         keys[0].value);
      };
    } else if (leaf == "peer") {
      read_handler_ = [local_core](const std::string &cube_name,
                                   const ListKeyValues &keys) -> Response {
        return local_core->base_model()->get_port_peer(cube_name,
                                                       keys[0].value);
      };

      replace_handler_ = [local_core](
          const std::string &cube_name, const nlohmann::json &json,
          const ListKeyValues &keys, Endpoint::Operation op) -> Response {
        return local_core->base_model()->set_port_peer(cube_name, keys[0].value,
                                                       json);
      };
    } else {
      throw std::runtime_error("unkown element found in base datamodel: " +
                               leaf);
    }
  }

  return std::make_unique<LeafResource>(
      std::move(read_handler_), std::move(replace_handler_), name, description,
      cli_example, rest_endpoint, parent, configuration, init_only_config,
      core_, std::move(value_field), node_fields, mandatory, type,
      std::move(default_value), is_enum, values);
}

std::unique_ptr<Endpoint::LeafListResource> ConcreteFactory::RestLeafList(
    const std::queue<std::string> &tree_names, const std::string &name,
    const std::string &description, const std::string &cli_example,
    const std::string &rest_endpoint, const Body::ParentResource *parent,
    std::unique_ptr<Body::JsonValueField> &&value_field,
    const std::vector<Body::JsonNodeField> &node_fields, bool configuration,
    bool init_only_config, bool mandatory, Types::Scalar type,
    std::vector<std::string> &&default_value) const {
  throw std::invalid_argument(
      "Yang case leaf-list not supported in base datamodel");
}

std::unique_ptr<Endpoint::ListResource> ConcreteFactory::RestList(
    const std::queue<std::string> &tree_names, const std::string &name,
    const std::string &description, const std::string &cli_example,
    const std::string &rest_endpoint,
    const std::string &rest_endpoint_whole_list,
    const Body::ParentResource *parent, bool configuration,
    bool init_only_config, std::vector<Resources::Body::ListKey> &&keys,
    const std::vector<Body::JsonNodeField> &node_fields) const {
  throw std::invalid_argument(
      "Yang case rest-list not supported in base datamodel");
}

std::unique_ptr<Endpoint::ParentResource> ConcreteFactory::RestGeneric(
    const std::queue<std::string> &tree_names, const std::string &name,
    const std::string &description, const std::string &cli_example,
    const std::string &rest_endpoint, const Body::ParentResource *parent,
    const std::vector<Body::JsonNodeField> &node_fields, bool configuration,
    bool init_only_config, bool container_presence, bool rpc_action) const {
  throw std::invalid_argument(
      "Yang case rest-generic not supported in base datamodel");
}

std::unique_ptr<Endpoint::Service> ConcreteFactory::RestService(
    [[maybe_unused]] const std::queue<std::string> &tree_names,
    const std::string &name, const std::string &description,
    const std::string &cli_example, std::string base_endpoint,
    std::string version) const {
  throw std::invalid_argument(
      "Yang case rest-service not supported in base datamodel");
}
}  // namespace polycube::polycubed::Rest::Resources::Data::BaseModel
