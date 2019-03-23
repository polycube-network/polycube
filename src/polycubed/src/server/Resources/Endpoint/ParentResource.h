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
#include <string>
#include <vector>

#include "../Body/ParentResource.h"
#include "Resource.h"

namespace polycube::polycubed::Rest::Resources::Endpoint {
using Pistache::Http::ResponseWriter;
using Pistache::Rest::Request;

class ParentResource : public Resource, public virtual Body::ParentResource {
 public:
  ParentResource(const std::string &name, const std::string &description,
                 const std::string &cli_example,
                 const std::string &rest_endpoint,
                 const Body::ParentResource *const parent, PolycubedCore *core,
                 const std::vector<Body::JsonNodeField> &node_fields,
                 bool configuration, bool init_only_config,
                 bool container_presence, bool rpc_action);

  ~ParentResource() override;

  std::vector<Response> RequestValidate(
      const Pistache::Rest::Request &request,
      const std::string &caller_name) const override;

  void CreateReplaceUpdate(const Pistache::Rest::Request &request,
                           Pistache::Http::ResponseWriter response, bool update,
                           bool initialization) final;

  void Keys(const Pistache::Rest::Request &request,
            ListKeyValues &parsed) const override;

  virtual Response DeleteValue(const std::string &cube_name,
                               const ListKeyValues &keys) = 0;
  Response Help(const std::string &cube_name, HelpType type,
                const ListKeyValues &keys);

  Response Completion(const std::string &cube_name, HelpType type,
                      const ListKeyValues &keys);

 protected:
  /**
   * Used by ChoiceResource and CaseResource: no endpoint
   */
  ParentResource(const std::string &name, const std::string &description,
                 const std::string &cli_example,
                 const Body::ParentResource *const parent, PolycubedCore *core,
                 const std::vector<Body::JsonNodeField> &node_fields);

  /*
   * Help related methods
   */
  nlohmann::json helpElements(bool mark_as_keys = false) const;
  nlohmann::json helpWritableLeafs(bool include_init_only = false) const;
  nlohmann::json helpComplexElements() const;
  std::vector<std::string> helpActions() const;

  std::vector<std::string> completionElements() const;
  std::vector<std::string> completionWritableLeafs(bool include_init_only = false) const;
  std::vector<std::string> completionComplexElements() const;
  std::vector<std::string> completionActions() const;
  std::vector<std::string> completionActionElements(bool include_init_only = false) const;

 private:
  const bool has_endpoints_;
  const bool rpc_action_;

  void get(const Request &request, ResponseWriter response);

  virtual void post(const Request &request, ResponseWriter response);

  virtual void put(const Request &request, ResponseWriter response);

  virtual void patch(const Request &request, ResponseWriter response);

  virtual void del(const Request &request, ResponseWriter response);

  void options(const Request &request, ResponseWriter response);
};
}  // namespace polycube::polycubed::Rest::Resources::Endpoint
