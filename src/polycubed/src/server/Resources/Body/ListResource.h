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

#include "ParentResource.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace polycube::polycubed::Rest::Resources::Body {
class ListKey;

class ListResource : public virtual ParentResource {
 public:
  ListResource(const std::string &name, const std::string &description,
               const std::string &cli_example,
               const ParentResource *const parent, PolycubedCore *core,
               std::vector<ListKey> &&keys,
               const std::vector<JsonNodeField> &node_fields,
               bool configuration, bool init_only_config);

  bool ValidateKeys(std::unordered_map<std::string, std::string> keys) const;

  const Response ReadValue(const std::string &cube_name,
                           const ListKeyValues &keys) const override;

  virtual const Response ReadWhole(const std::string &cube_name,
                                   const ListKeyValues &keys) const;

  bool IsMandatory() const override;

  void SetDefaultIfMissing(nlohmann::json &body,
                           bool initialization) const final;

  const std::vector<ListKey> keys_;
};
}  // namespace polycube::polycubed::Rest::Resources::Body
