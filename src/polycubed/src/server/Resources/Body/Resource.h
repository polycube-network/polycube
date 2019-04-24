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

#include "ListKey.h"

#include "polycube/services/json.hpp"
#include "polycube/services/response.h"

namespace polycube::polycubed {
class PolycubedCore;

namespace Rest::Resources::Body {
class ParentResource;
class Service;
class JsonNodeField;
}  // namespace Rest::Resources::Body
}  // namespace polycube::polycubed

namespace polycube::polycubed::Rest::Resources::Body {

class Resource {
 public:
  Resource(const std::string &name, const std::string &description,
           const std::string &cli_example, const ParentResource *const parent,
           PolycubedCore *core, bool configuration, bool init_only_config,
           const std::vector<JsonNodeField> &node_fields);

  Resource(const Resource &) = delete;

  Resource &operator=(const Resource &) = delete;

  virtual ~Resource() = default;

  virtual bool IsMandatory() const = 0;

  virtual bool IsConfiguration() const = 0;

  const bool IsInitOnlyConfig() const {
    return init_only_config_;
  }

  const std::string &Name() const {
    return name_;
  }

  const bool IsKey() const {
    return is_key_;
  }

  void SetIsKey(bool is_key) {
    is_key_ = is_key;
  }

  const std::string &Description() const {
    return description_;
  }

  const std::string &CliExample() const {
    return cli_example_;
  }

  const std::shared_ptr<Service> GetService(
      const std::string &service_name) const;

  virtual std::vector<Response> BodyValidate(const std::string &cube_name,
                                             const ListKeyValues &keys,
                                             nlohmann::json &body,
                                             bool initialization) const = 0;

  virtual void SetDefaultIfMissing(nlohmann::json &body) const = 0;

  inline const Body::ParentResource *const Parent() const {
    return parent_;
  }

  virtual const Response ReadValue(const std::string &cube_name,
                                   const ListKeyValues &keys) const;

  virtual nlohmann::json ToHelpJson() const;

 protected:
  const std::string name_;
  const std::string description_;
  const std::string cli_example_;
  bool is_key_;
  const Body::ParentResource *const parent_;
  const bool configuration_;
  const bool init_only_config_;
  PolycubedCore *const core_;
  const std::vector<JsonNodeField> node_fields_;
};
}  // namespace polycube::polycubed::Rest::Resources::Body
