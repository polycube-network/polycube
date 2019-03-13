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
#include "ConcreteFactory.h"

#include <dlfcn.h>

#include <memory>
#include <string>
#include <vector>

#include "../../Body/JsonNodeField.h"

#include "../../Endpoint/LeafResource.h"
#include "../../Endpoint/ListResource.h"
#include "../../Endpoint/ParentResource.h"
#include "../../Endpoint/Service.h"

#include "EntryPoint.h"
#include "LeafResource.h"
#include "ListResource.h"
#include "ParentResource.h"
#include "Service.h"

namespace polycube::polycubed::Rest::Resources::Data::Lib {
using EntryPoint::GenerateHandlerName;
using EntryPoint::GenerateHelpName;
using EntryPoint::Operation;

ConcreteFactory::ConcreteFactory(const std::string &file_name,
                                 PolycubedCore *core)
    : AbstractFactory(core),
      handle_{::dlopen(file_name.data(), RTLD_NOW), [](void *h) {
                if (h != nullptr)
                  ::dlclose(h);
              }} {
  if (!handle_) {
    throw std::logic_error("Cannot load service implementation " + file_name);
  }
}

template <typename T>
std::function<T> ConcreteFactory::LoadHandler(
    const std::string &function_name) const {
  void *result = ::dlsym(handle_.get(), function_name.data());
  if (!result) {
    char *const error = ::dlerror();
    if (error) {
      throw std::domain_error(std::string{error});
    }
  }
  return reinterpret_cast<T *>(result);
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

std::unique_ptr<Endpoint::LeafResource> ConcreteFactory::RestLeaf(
    const std::queue<std::string> &tree_names, const std::string &name,
    const std::string &description, const std::string &cli_example,
    const std::string &rest_endpoint, const Body::ParentResource *parent,
    std::unique_ptr<Body::JsonValueField> &&value_field,
    const std::vector<Body::JsonNodeField> &node_fields, bool configuration,
    bool init_only_config, bool mandatory, Types::Scalar type,
    std::unique_ptr<const std::string> &&default_value) const {
  auto read_handler = LoadHandler<Response(const char *, const Key *, size_t)>(
      GenerateHandlerName(tree_names, Operation::kRead));
  if (!configuration || init_only_config) {
    return std::make_unique<LeafResource>(
        std::move(read_handler), name, description, cli_example, rest_endpoint,
        parent, configuration, init_only_config, core_, std::move(value_field),
        node_fields, mandatory, type, std::move(default_value));
  }
  auto replace_handler =
      LoadHandler<Response(const char *, const Key *, size_t, const char *)>(
          GenerateHandlerName(tree_names, Operation::kUpdate));
  return std::make_unique<LeafResource>(
      std::move(replace_handler), std::move(read_handler), name, description,
      cli_example, rest_endpoint, parent, core_, std::move(value_field),
      node_fields, mandatory, type, std::move(default_value));
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
      "Yang case leaf-list not supported with shared object protocol.");
}

std::unique_ptr<Endpoint::ListResource> ConcreteFactory::RestList(
    const std::queue<std::string> &tree_names, const std::string &name,
    const std::string &description, const std::string &cli_example,
    const std::string &rest_endpoint,
    const std::string &rest_endpoint_whole_list,
    const Body::ParentResource *parent, bool configuration,
    bool init_only_config, std::vector<Resources::Body::ListKey> &&keys,
    const std::vector<Body::JsonNodeField> &node_fields) const {
  auto sname = name;
  auto length = sname.length();
  std::replace(std::begin(sname), std::end(sname), '-', '_');
  std::replace(std::begin(sname), std::end(sname), '.', '_');
  std::string replace = sname + "_list";

  auto read_name = GenerateHandlerName(tree_names, Operation::kRead);
  auto read_entry_handler =
      LoadHandler<Response(const char *, const Key *, size_t)>(read_name);

  auto help_name = GenerateHelpName(tree_names);
  auto help =
      LoadHandler<Response(HelpType, const char *name, const Key *, size_t)>(
          help_name);

  help_name.replace(help_name.rfind(sname), length, replace);
  auto help_multiple =
      LoadHandler<Response(HelpType, const char *name, const Key *, size_t)>(
          help_name);

  read_name.replace(read_name.rfind(sname), length, replace);
  auto read_whole_handler =
      LoadHandler<Response(const char *, const Key *, size_t)>(read_name);
  if (!configuration || init_only_config) {
    return std::make_unique<ListResource>(
        std::move(read_entry_handler), std::move(read_whole_handler),
        std::move(help), std::move(help_multiple), name, description,
        cli_example, rest_endpoint, std::move(rest_endpoint_whole_list), parent,
        configuration, init_only_config, core_, std::move(keys), node_fields);
  }

  auto create_name = GenerateHandlerName(tree_names, Operation::kCreate);
  auto create_entry_handler =
      LoadHandler<Response(const char *, const Key *, size_t, const char *)>(
          create_name);

  auto replace_name = GenerateHandlerName(tree_names, Operation::kReplace);
  auto replace_entry_handler =
      LoadHandler<Response(const char *, const Key *, size_t, const char *)>(
          replace_name);

  auto update_name = GenerateHandlerName(tree_names, Operation::kUpdate);
  auto update_entry_handler =
      LoadHandler<Response(const char *, const Key *, size_t, const char *)>(
          update_name);

  auto delete_name = GenerateHandlerName(tree_names, Operation::kDelete);
  auto delete_entry_handler =
      LoadHandler<Response(const char *, const Key *, size_t)>(delete_name);

  create_name.replace(create_name.rfind(sname), length, replace);
  replace_name.replace(replace_name.rfind(sname), length, replace);
  update_name.replace(update_name.rfind(sname), length, replace);
  delete_name.replace(delete_name.rfind(sname), length, replace);

  auto create_whole_handler =
      LoadHandler<Response(const char *, const Key *, size_t, const char *)>(
          create_name);
  auto replace_whole_handler =
      LoadHandler<Response(const char *, const Key *, size_t, const char *)>(
          replace_name);
  auto update_whole_handler =
      LoadHandler<Response(const char *, const Key *, size_t, const char *)>(
          update_name);
  auto delete_whole_handler =
      LoadHandler<Response(const char *, const Key *, size_t)>(delete_name);

  return std::make_unique<ListResource>(
      std::move(create_entry_handler), std::move(replace_entry_handler),
      std::move(update_entry_handler), std::move(read_entry_handler),
      std::move(delete_entry_handler), std::move(create_whole_handler),
      std::move(replace_whole_handler), std::move(update_whole_handler),
      std::move(read_whole_handler), std::move(delete_whole_handler),
      std::move(help), std::move(help_multiple), name, description, cli_example,
      rest_endpoint, std::move(rest_endpoint_whole_list), parent, core_,
      std::move(keys), node_fields);
}

std::unique_ptr<Endpoint::ParentResource> ConcreteFactory::RestGeneric(
    const std::queue<std::string> &tree_names, const std::string &name,
    const std::string &description, const std::string &cli_example,
    const std::string &rest_endpoint, const Body::ParentResource *parent,
    const std::vector<Body::JsonNodeField> &node_fields, bool configuration,
    bool init_only_config, bool container_presence, bool rpc_action) const {
  auto help_name = GenerateHelpName(tree_names);
  auto help =
      LoadHandler<Response(HelpType, const char *name, const Key *, size_t)>(
          help_name);

  if (rpc_action) {
    auto create_handler =
        LoadHandler<Response(const char *, const Key *, size_t, const char *)>(
            GenerateHandlerName(tree_names, Operation::kCreate));
    return std::make_unique<ParentResource>(
        std::move(create_handler), std::move(help), name, description,
        cli_example, rest_endpoint, parent, core_, node_fields);
  }

  auto read_handler = LoadHandler<Response(const char *, const Key *, size_t)>(
      GenerateHandlerName(tree_names, Operation::kRead));
  if (!configuration || init_only_config) {
    return std::make_unique<ParentResource>(
        std::move(read_handler), std::move(help), name, description,
        cli_example, rest_endpoint, parent, configuration, init_only_config,
        core_, node_fields, container_presence);
  }

  auto create_handler =
      LoadHandler<Response(const char *, const Key *, size_t, const char *)>(
          GenerateHandlerName(tree_names, Operation::kCreate));
  auto replace_handler =
      LoadHandler<Response(const char *, const Key *, size_t, const char *)>(
          GenerateHandlerName(tree_names, Operation::kReplace));
  auto update_handler =
      LoadHandler<Response(const char *, const Key *, size_t, const char *)>(
          GenerateHandlerName(tree_names, Operation::kUpdate));
  auto delete_handler =
      LoadHandler<Response(const char *, const Key *, size_t)>(
          GenerateHandlerName(tree_names, Operation::kDelete));

  return std::make_unique<ParentResource>(
      std::move(create_handler), std::move(replace_handler),
      std::move(update_handler), std::move(read_handler),
      std::move(delete_handler), std::move(help), name, description,
      cli_example, rest_endpoint, parent, core_, node_fields,
      container_presence);
}

std::unique_ptr<Endpoint::Service> ConcreteFactory::RestService(
    [[maybe_unused]] const std::queue<std::string> &tree_names,
    const std::string &name, const std::string &description,
    const std::string &cli_example, std::string base_endpoint,
    std::string version) const {
  std::string create_single{"create_" + name + "_by_id_handler"};
  std::string update_single{"update_" + name + "_by_id_handler"};
  std::string read_single{"read_" + name + "_by_id_handler"};
  std::string replace_single{"replace_" + name + "_by_id_handler"};
  std::string delete_single{"delete_" + name + "_by_id_handler"};

  std::string read_whole{"read_" + name + "_list_by_id_handler"};
  std::string update_whole{"update_" + name + "_list_by_id_handler"};

  auto create_handler =
      LoadHandler<Response(const char *, const Key *, size_t, const char *)>(
          create_single);
  auto update_handler =
      LoadHandler<Response(const char *, const Key *, size_t, const char *)>(
          update_single);
  auto read_handler =
      LoadHandler<Response(const char *, const Key *, size_t)>(read_single);
  auto replace_handler =
      LoadHandler<Response(const char *, const Key *, size_t, const char *)>(
          replace_single);
  auto delete_handler =
      LoadHandler<Response(const char *, const Key *, size_t)>(delete_single);

  auto read_list_handler =
      LoadHandler<Response(const char *, const Key *, size_t)>(read_whole);
  auto update_list_handler =
      LoadHandler<Response(const char *, const Key *, size_t, const char *)>(
          update_whole);

  auto init_handler =
      LoadHandler<void(service::CubeFactory *, const char *)>("init");

  auto help_name = GenerateHelpName(tree_names);
  auto help =
      LoadHandler<Response(HelpType, const char *name, const Key *, size_t)>(
          help_name);

  std::string service_help_name{name + "_list_by_id_help"};
  auto service_help =
      LoadHandler<Response(HelpType, const char *name, const Key *, size_t)>(
          service_help_name);

  return std::make_unique<Service>(
      handle_, std::move(create_handler), std::move(update_handler),
      std::move(read_handler), std::move(replace_handler),
      std::move(delete_handler), std::move(read_list_handler),
      std::move(update_list_handler), std::move(init_handler), std::move(help),
      std::move(service_help), name, description, cli_example,
      std::move(base_endpoint), std::move(version), core_);
}
}  // namespace polycube::polycubed::Rest::Resources::Data::Lib
