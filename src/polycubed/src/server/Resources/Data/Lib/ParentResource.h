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
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "../../Endpoint/ParentResource.h"
#include "EntryPoint.h"
#include "polycube/services/shared_lib_elements.h"

namespace polycube::polycubed::Rest::Resources::Data::Lib {

class ParentResource : public Endpoint::ParentResource {
 public:
  ParentResource(
      std::function<Response(const char *, const Key *, size_t, const char *)>
          create_handler,
      std::function<Response(const char *, const Key *, size_t, const char *)>
          replace_handler,
      std::function<Response(const char *, const Key *, size_t, const char *)>
          update_handler,
      std::function<Response(const char *, const Key *, size_t)> read_handler,
      std::function<Response(const char *, const Key *, size_t)> delete_handler,
      std::function<Response(HelpType, const char *name, const Key *, size_t)>
          help,
      const std::string &name, const std::string &description,
      const std::string &cli_example, const std::string &rest_endpoint,
      const Body::ParentResource *parent, PolycubedCore *core,
      const std::vector<Body::JsonNodeField> &node_fields,
      bool container_presence = false);

  ParentResource(
      std::function<Response(const char *, const Key *, size_t, const char *)>
          create_handler,
      std::function<Response(HelpType, const char *name, const Key *, size_t)>
          help,
      const std::string &name, const std::string &description,
      const std::string &cli_example, const std::string &rest_endpoint,
      const Body::ParentResource *parent, PolycubedCore *core,
      const std::vector<Body::JsonNodeField> &node_fields);

  ParentResource(
      std::function<Response(const char *, const Key *, size_t)> read_handler,
      std::function<Response(HelpType, const char *name, const Key *, size_t)>
          help,
      const std::string &name, const std::string &description,
      const std::string &cli_example, const std::string &rest_endpoint,
      const Body::ParentResource *parent, bool configuration,
      bool init_only_config, PolycubedCore *core,
      const std::vector<Body::JsonNodeField> &node_fields,
      bool container_presence = false);

  const Response ReadValue(const std::string &cube_name,
                           const ListKeyValues &keys) const final;

  Response WriteValue(const std::string &cube_name, const nlohmann::json &value,
                      const ListKeyValues &keys,
                      Endpoint::Operation operation) final;

  Response DeleteValue(const std::string &cube_name,
                       const ListKeyValues &keys) final;

  Response Help(const std::string &cube_name, HelpType type,
                const ListKeyValues &keys) final;

 private:
  const std::function<Response(const char *, const Key *, size_t, const char *)>
      create_handler_;
  const std::function<Response(const char *, const Key *, size_t, const char *)>
      replace_handler_;
  const std::function<Response(const char *, const Key *, size_t, const char *)>
      update_handler_;
  const std::function<Response(const char *, const Key *, size_t)>
      read_handler_;
  const std::function<Response(const char *, const Key *, size_t)>
      delete_handler_;
  const std::function<Response(HelpType, const char *name, const Key *, size_t)>
      help_;
};
}  // namespace polycube::polycubed::Rest::Resources::Data::Lib
