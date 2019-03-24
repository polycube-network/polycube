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
#include "ParentResource.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "../../Body/JsonNodeField.h"
#include "KeyListArray.h"

namespace polycube::polycubed::Rest::Resources::Data::Lib {

ParentResource::ParentResource(
    std::function<Response(const char *, const Key *, size_t, const char *)>
        create_handler,
    std::function<Response(const char *, const Key *, size_t, const char *)>
        replace_handler,
    std::function<Response(const char *, const Key *, size_t, const char *)>
        update_handler,
    std::function<Response(const char *, const Key *, size_t)> read_handler,
    std::function<Response(const char *, const Key *, size_t)> delete_handler,
    const std::string &name, const std::string &description,
    const std::string &cli_example, const std::string &rest_endpoint,
    const Body::ParentResource *parent, PolycubedCore *core,
    const std::vector<Body::JsonNodeField> &node_fields,
    bool container_presence)
    : Body::ParentResource(name, description, cli_example, parent, core,
                           node_fields, true, false, container_presence),
      Endpoint::ParentResource(name, description, cli_example, rest_endpoint,
                               parent, core, node_fields, true, false, false,
                               false),
      create_handler_{std::move(create_handler)},
      replace_handler_{std::move(replace_handler)},
      update_handler_{std::move(update_handler)},
      read_handler_{std::move(read_handler)},
      delete_handler_{std::move(delete_handler)} {}

ParentResource::ParentResource(
    std::function<Response(const char *, const Key *, size_t, const char *)>
        create_handler,
    const std::string &name, const std::string &description,
    const std::string &cli_example, const std::string &rest_endpoint,
    const Body::ParentResource *parent, PolycubedCore *core,
    const std::vector<Body::JsonNodeField> &node_fields)
    : Body::ParentResource(name, description, cli_example, parent, core,
                           node_fields, false, false, false, true),
      Endpoint::ParentResource(name, description, cli_example, rest_endpoint,
                               parent, core, node_fields, false, false, false,
                               true),
      create_handler_{std::move(create_handler)},
      replace_handler_{},
      update_handler_{},
      read_handler_{},
      delete_handler_{} {}

ParentResource::ParentResource(
    std::function<Response(const char *, const Key *, size_t)> read_handler,
    const std::string &name, const std::string &description,
    const std::string &cli_example, const std::string &rest_endpoint,
    const Body::ParentResource *parent, bool configuration,
    bool init_only_config, PolycubedCore *core,
    const std::vector<Body::JsonNodeField> &node_fields,
    bool container_presence)
    : Body::ParentResource(name, description, cli_example, parent, core,
                           node_fields, configuration, init_only_config,
                           container_presence),
      Endpoint::ParentResource(name, description, cli_example, rest_endpoint,
                               parent, core, node_fields, configuration,
                               init_only_config, false, false),
      create_handler_{},
      replace_handler_{},
      update_handler_{},
      read_handler_{std::move(read_handler)},
      delete_handler_{} {}

const Response ParentResource::ReadValue(const std::string &cube_name,
                                         const ListKeyValues &keys) const {
  const auto &key_params = KeyListArray::Generate(keys);
  return read_handler_(cube_name.data(), key_params.data(), key_params.size());
}

Response ParentResource::WriteValue(const std::string &cube_name,
                                    const nlohmann::json &value,
                                    const ListKeyValues &keys,
                                    Endpoint::Operation operation) {
  const auto &key_params = KeyListArray::Generate(keys);
  switch (operation) {
  case Endpoint::Operation::kCreate:
    return create_handler_(cube_name.data(), key_params.data(),
                           key_params.size(), value.dump().data());
  case Endpoint::Operation::kReplace:
    return replace_handler_(cube_name.data(), key_params.data(),
                            key_params.size(), value.dump().data());
  case Endpoint::Operation::kUpdate:
    return update_handler_(cube_name.data(), key_params.data(),
                           key_params.size(), value.dump().data());
  }
}

Response ParentResource::DeleteValue(const std::string &cube_name,
                                     const ListKeyValues &keys) {
  const auto &key_params = KeyListArray::Generate(keys);
  return delete_handler_(cube_name.data(), key_params.data(),
                         key_params.size());
}
}  // namespace polycube::polycubed::Rest::Resources::Data::Lib
