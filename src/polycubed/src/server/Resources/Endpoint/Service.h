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

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../Validators/InSetValidator.h"
#include "../Body/Service.h"
#include "ParentResource.h"

namespace polycube::service {
class CubeFactory;
}

namespace polycube::polycubed::Rest::Resources::Endpoint {
using Pistache::Http::ResponseWriter;
using Pistache::Rest::Request;

class Service : public ParentResource, public Body::Service {
 public:
  Service(const std::string &name, const std::string &description,
          const std::string &cli_example, std::string base_address,
          const std::string &version, PolycubedCore *core);

  ~Service() override;

  virtual void init(service::CubeFactory *factory,
                    const std::string &log_file) = 0;

  static const std::string Cube(const Pistache::Rest::Request &request);

  void ClearCubes();

  Response Help(HelpType type);

  Response CompletionService(HelpType type);

  virtual Response ReadHelp() = 0;

  std::vector<Response> CreateReplaceUpdate(const std::string &name, nlohmann::json &body,
                                            bool update, bool initialization);

 private:
  const std::string body_rest_endpoint_;
  Validators::InSetValidator cube_names_;

  nlohmann::json getServiceKeys() const;

  void post(const Request &request, ResponseWriter response) final;

  void put(const Request &request, ResponseWriter response) final;

  void patch(const Request &request, ResponseWriter response) final;

  void del(const Request &request, ResponseWriter response) final;

  void get_body(const Request &request, ResponseWriter response);

  void post_body(const Request &request, ResponseWriter response);

  void patch_body(const Request &request, ResponseWriter response);

  void options_body(const Request &request, ResponseWriter response);
};
}  // namespace polycube::polycubed::Rest::Resources::Endpoint
