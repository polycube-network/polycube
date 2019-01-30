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

#include <pistache/router.h>

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "Resource.h"

namespace polycube::polycubed::Rest::Resources::Body {
class ParentResource : public Resource {
 public:
  ParentResource(const std::string &name, const std::string &description,
                 const std::string &cli_example,
                 const ParentResource *const parent, PolycubedCore *core,
                 const std::vector<JsonNodeField> &node_fields,
                 bool configuration, bool init_only_config,
                 bool container_presence = false,
                 bool rpc_action = false);

  ~ParentResource() override = default;

  std::vector<Response> BodyValidate(const std::string &cube_name,
                                     const ListKeyValues &keys,
                                     nlohmann::json &body,
                                     bool initialization) const override;

  virtual void AddChild(std::shared_ptr<Resource> &&child);

  bool IsMandatory() const override;

  bool IsConfiguration() const final;

  void SetDefaultIfMissing(nlohmann::json &body,
                           bool initialization) const override;

  std::shared_ptr<Resource> Child(const std::string &child_name) const;

 protected:
  std::vector<std::shared_ptr<Resource>> children_;

 private:
  /**
   * MUST be set to true only if it is a container
   * and it has presence flag (or explicitly set to false).
   */
  const bool container_presence_;
  const bool rpc_action_;
};
}  // namespace polycube::polycubed::Rest::Resources::Body
