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
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "../../Endpoint/LeafResource.h"
#include "EntryPoint.h"
#include "polycube/services/shared_lib_elements.h"

namespace polycube::polycubed::Rest::Resources::Endpoint {
class ParentResource;
}

namespace polycube::polycubed::Rest::Resources::Data::Lib {

class LeafResource : public Endpoint::LeafResource {
 public:
  LeafResource(
      std::function<Response(const char *, const Key *, size_t)> read_handler,
      const std::string &name, const std::string &description,
      const std::string &cli_example, const std::string &rest_endpoint,
      const Body::ParentResource *parent, bool configuration,
      bool init_only_config, PolycubedCore *core,
      std::unique_ptr<Body::JsonValueField> &&value_field,
      const std::vector<Body::JsonNodeField> &node_fields, bool mandatory,
      Types::Scalar type, std::unique_ptr<const std::string> &&default_value);

  LeafResource(
      std::function<Response(const char *, const Key *, size_t, const char *)>
          replace_handler,
      std::function<Response(const char *, const Key *, size_t)> read_handler,
      const std::string &name, const std::string &description,
      const std::string &cli_example, const std::string &rest_endpoint,
      const Body::ParentResource *parent, PolycubedCore *core,
      std::unique_ptr<Body::JsonValueField> &&value_field,
      const std::vector<Body::JsonNodeField> &node_fields, bool mandatory,
      Types::Scalar type, std::unique_ptr<const std::string> &&default_value);

  const Response ReadValue(const std::string &cube_name,
                           const ListKeyValues &keys) const final;

  Response WriteValue(const std::string &cube_name, const nlohmann::json &value,
                      const ListKeyValues &keys,
                      Endpoint::Operation operation) final;

 private:
  std::function<Response(const char *, const Key *, size_t)> read_handler_;
  std::function<Response(const char *, const Key *, size_t, const char *)>
      replace_handler_;
};
}  // namespace polycube::polycubed::Rest::Resources::Data::Lib
