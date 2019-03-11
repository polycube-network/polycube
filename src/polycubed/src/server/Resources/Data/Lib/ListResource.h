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

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../../Endpoint/ListResource.h"
#include "EntryPoint.h"
#include "ParentResource.h"
#include "polycube/services/shared_lib_elements.h"

namespace polycube::polycubed::Rest::Resources::Data::Lib {
class ListResource : public Endpoint::ListResource {
 public:
  ListResource(
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
      const std::vector<Body::JsonNodeField> &node_fields);

  ListResource(
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
      const std::vector<Body::JsonNodeField> &node_fields);

  const Response ReadValue(const std::string &cube_name,
                           const ListKeyValues &keys) const final;

  const Response ReadWhole(const std::string &cube_name,
                           const ListKeyValues &keys) const final;

  Response WriteValue(const std::string &cube_name, const nlohmann::json &value,
                      const ListKeyValues &keys,
                      Endpoint::Operation operation) final;

  Response DeleteValue(const std::string &cube_name,
                       const ListKeyValues &keys) final;

  Response WriteWhole(const std::string &cube_name, const nlohmann::json &value,
                      const ListKeyValues &keys,
                      Endpoint::Operation operation) final;

  Response DeleteWhole(const std::string &cube_name,
                       const ListKeyValues &keys) final;

  Response Help(const std::string &cube_name, HelpType type,
                const ListKeyValues &keys) final;
  Response HelpMultiple(const std::string &cube_name, HelpType type,
                        const ListKeyValues &keys);

 private:
  const std::function<Response(const char *, const Key *, size_t, const char *)>
      create_entry_handler_;
  const std::function<Response(const char *, const Key *, size_t, const char *)>
      replace_entry_handler_;
  const std::function<Response(const char *, const Key *, size_t, const char *)>
      update_entry_handler_;
  const std::function<Response(const char *, const Key *, size_t)>
      read_entry_handler_;
  const std::function<Response(const char *, const Key *, size_t)>
      delete_entry_handler_;
  const std::function<Response(const char *, const Key *, size_t, const char *)>
      create_whole_handler_;
  const std::function<Response(const char *, const Key *, size_t, const char *)>
      replace_whole_handler_;
  const std::function<Response(const char *, const Key *, size_t, const char *)>
      update_whole_handler_;
  const std::function<Response(const char *, const Key *, size_t)>
      read_whole_handler_;
  const std::function<Response(const char *, const Key *, size_t)>
      delete_whole_handler_;
  const std::function<Response(HelpType, const char *name, const Key *, size_t)>
      help_;
  const std::function<Response(HelpType, const char *name, const Key *, size_t)>
      help_multiple_;
};
}  // namespace polycube::polycubed::Rest::Resources::Data::Lib
