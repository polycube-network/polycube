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
#include "ListResource.h"

#include <string>

#include "../../Body/JsonNodeField.h"
#include "KeyListArray.h"

namespace polycube::polycubed::Rest::Resources::Data::Lib {
ListResource::ListResource(
    std::function<Response(const char *, const Key *, size_t, const char *)>
        create_entry_handler,
    std::function<Response(const char *, const Key *, size_t, const char *)>
        replace_entry_handler,
    std::function<Response(const char *, const Key *, size_t, const char *)>
        update_entry_handler,
    std::function<Response(const char *, const Key *, size_t)>
        read_entry_handler,
    std::function<Response(const char *, const Key *, size_t)>
        delete_entry_handler,
    std::function<Response(const char *, const Key *, size_t, const char *)>
        create_whole_handler,
    std::function<Response(const char *, const Key *, size_t, const char *)>
        replace_whole_handler,
    std::function<Response(const char *, const Key *, size_t, const char *)>
        update_whole_handler,
    std::function<Response(const char *, const Key *, size_t)>
        read_whole_handler,
    std::function<Response(const char *, const Key *, size_t)>
        delete_whole_handler,
    std::function<Response(HelpType, const char *name, const Key *, size_t)>
        help,
    std::function<Response(HelpType, const char *name, const Key *, size_t)>
        help_multiple,
    const std::string &name, const std::string &description,
    const std::string &cli_example, const std::string &rest_endpoint,
    const std::string &rest_endpoint_multiple,
    const Body::ParentResource *parent, PolycubedCore *core,
    std::vector<Body::ListKey> &&keys,
    const std::vector<Body::JsonNodeField> &node_fields)
    : Body::ParentResource(name, description, cli_example, parent, core,
                           node_fields, true, false, false),
      Endpoint::ListResource(name, description, cli_example, parent, core,
                             rest_endpoint, rest_endpoint_multiple,
                             std::move(keys), node_fields, true, false),
      create_entry_handler_{std::move(create_entry_handler)},
      replace_entry_handler_{std::move(replace_entry_handler)},
      update_entry_handler_{std::move(update_entry_handler)},
      read_entry_handler_{std::move(read_entry_handler)},
      delete_entry_handler_{std::move(delete_entry_handler)},
      create_whole_handler_{std::move(create_whole_handler)},
      replace_whole_handler_{std::move(replace_whole_handler)},
      update_whole_handler_{std::move(update_whole_handler)},
      read_whole_handler_{std::move(read_whole_handler)},
      delete_whole_handler_{std::move(delete_whole_handler)},
      help_{std::move(help)},
      help_multiple_{std::move(help_multiple)} {}

ListResource::ListResource(
    std::function<Response(const char *, const Key *, size_t)>
        read_entry_handler,
    std::function<Response(const char *, const Key *, size_t)>
        read_whole_handler,
    std::function<Response(HelpType, const char *name, const Key *, size_t)>
        help,
    std::function<Response(HelpType, const char *name, const Key *, size_t)>
        help_multiple,
    const std::string &name, const std::string &description,
    const std::string &cli_example, const std::string &rest_endpoint,
    const std::string &rest_endpoint_multiple,
    const Body::ParentResource *parent, bool configuration,
    bool init_only_config, PolycubedCore *core,
    std::vector<Body::ListKey> &&keys,
    const std::vector<Body::JsonNodeField> &node_fields)
    : Body::ParentResource(name, description, cli_example, parent, core,
                           node_fields, configuration, init_only_config, false),
      Endpoint::ListResource(name, description, cli_example, parent, core,
                             rest_endpoint, rest_endpoint_multiple,
                             std::move(keys), node_fields, true, false),
      create_entry_handler_{},
      replace_entry_handler_{},
      update_entry_handler_{},
      read_entry_handler_{std::move(read_entry_handler)},
      delete_entry_handler_{},
      create_whole_handler_{},
      replace_whole_handler_{},
      update_whole_handler_{},
      read_whole_handler_{std::move(read_whole_handler)},
      delete_whole_handler_{},
      help_{std::move(help)},
      help_multiple_{std::move(help_multiple)} {}

const Response ListResource::ReadValue(const std::string &cube_name,
                                       const ListKeyValues &keys) const {
  const auto &key_params = KeyListArray::Generate(keys);
  return read_entry_handler_(cube_name.data(), key_params.data(),
                             key_params.size());
}

const Response ListResource::ReadWhole(const std::string &cube_name,
                                       const ListKeyValues &keys) const {
  const auto &key_params = KeyListArray::Generate(keys);
  return read_whole_handler_(cube_name.data(), key_params.data(),
                             key_params.size());
}

Response ListResource::WriteValue(const std::string &cube_name,
                                  const nlohmann::json &value,
                                  const ListKeyValues &keys,
                                  Endpoint::Operation operation) {
  const auto &key_params = KeyListArray::Generate(keys);
  switch (operation) {
  case Endpoint::Operation::kCreate:
    return create_entry_handler_(cube_name.data(), key_params.data(),
                                 key_params.size(), value.dump().data());
  case Endpoint::Operation::kReplace:
    return replace_entry_handler_(cube_name.data(), key_params.data(),
                                  key_params.size(), value.dump().data());
  case Endpoint::Operation::kUpdate:
    return update_entry_handler_(cube_name.data(), key_params.data(),
                                 key_params.size(), value.dump().data());
  }
}

Response ListResource::DeleteValue(const std::string &cube_name,
                                   const ListKeyValues &keys) {
  const auto &key_params = KeyListArray::Generate(keys);
  return delete_entry_handler_(cube_name.data(), key_params.data(),
                               key_params.size());
}

Response ListResource::WriteWhole(const std::string &cube_name,
                                  const nlohmann::json &value,
                                  const ListKeyValues &keys,
                                  Endpoint::Operation operation) {
  const auto &key_params = KeyListArray::Generate(keys);
  switch (operation) {
  case Endpoint::Operation::kCreate:
    return create_whole_handler_(cube_name.data(), key_params.data(),
                                 key_params.size(), value.dump().data());
  case Endpoint::Operation::kReplace:
    return replace_whole_handler_(cube_name.data(), key_params.data(),
                                  key_params.size(), value.dump().data());
  case Endpoint::Operation::kUpdate:
    return update_whole_handler_(cube_name.data(), key_params.data(),
                                 key_params.size(), value.dump().data());
  }
}

Response ListResource::DeleteWhole(const std::string &cube_name,
                                   const ListKeyValues &keys) {
  const auto &key_params = KeyListArray::Generate(keys);
  return delete_whole_handler_(cube_name.data(), key_params.data(),
                               key_params.size());
}

Response ListResource::Help(const std::string &cube_name, HelpType type,
                            const ListKeyValues &keys) {
  const auto &key_params = KeyListArray::Generate(keys);
  return help_(type, cube_name.data(), key_params.data(), key_params.size());
}

Response ListResource::HelpMultiple(const std::string &cube_name, HelpType type,
                                    const ListKeyValues &keys) {
  const auto &key_params = KeyListArray::Generate(keys);
  return help_multiple_(type, cube_name.data(), key_params.data(),
                        key_params.size());
}

}  // namespace polycube::polycubed::Rest::Resources::Data::Lib
