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
#include "Service.h"

#include "spdlog/spdlog.h"

#include "../../Body/JsonNodeField.h"
#include "KeyListArray.h"

namespace polycube::polycubed::Rest::Resources::Data::Lib {

Service::Service(
    std::shared_ptr<void> handle,
    std::function<Response(const char *, const Key *, size_t, const char *)>
        create_handler,
    std::function<Response(const char *, const Key *, size_t, const char *)>
        update_handler,
    std::function<Response(const char *, const Key *, size_t)> read_handler,
    std::function<Response(const char *, const Key *, size_t, const char *)>
        replace_handler,
    std::function<Response(const char *, const Key *, size_t)> delete_handler,
    std::function<Response(const char *, const Key *, size_t)>
        read_list_handler,
    std::function<Response(const char *, const Key *, size_t, const char *)>
        update_list_handler,
    std::function<void(service::CubeFactory *, const char *)> init_handler,
    std::function<Response()> service_help, const std::string &name,
    const std::string &description, const std::string &cli_example,
    std::string base_address, std::string version, PolycubedCore *core)
    : Body::ParentResource(name, description, cli_example, nullptr, core,
                           std::vector<Body::JsonNodeField>{}, true, false),
      Endpoint::Service(name, description, cli_example, std::move(base_address),
                        std::move(version), core),
      handle_{std::move(handle)},
      create_handler_{std::move(create_handler)},
      update_handler_{std::move(update_handler)},
      read_handler_{std::move(read_handler)},
      replace_handler_{std::move(replace_handler)},
      delete_handler_{std::move(delete_handler)},
      read_list_handler_{std::move(read_list_handler)},
      update_list_handler_{std::move(update_list_handler)},
      init_handler_{std::move(init_handler)},
      service_help_{std::move(service_help)} {
  spdlog::get("polycubed")->trace("loading {0} shared object", name_);
}

Service::~Service() {
  spdlog::get("polycubed")->trace("unloading {0} shared object", name_);
}

const Response Service::ReadValue(
    const std::string &cube_name,
    [[maybe_unused]] const ListKeyValues &keys) const {
  return read_handler_(cube_name.data(), nullptr, 0);
}

Response Service::WriteValue(const std::string &cube_name,
                             const nlohmann::json &value,
                             [[maybe_unused]] const ListKeyValues &keys,
                             Endpoint::Operation operation) {
  switch (operation) {
  case Endpoint::Operation::kCreate:
    return create_handler_(cube_name.data(), nullptr, 0, value.dump().data());
  case Endpoint::Operation::kReplace:
    return replace_handler_(cube_name.data(), nullptr, 0, value.dump().data());
  case Endpoint::Operation::kUpdate:
    return update_handler_(cube_name.data(), nullptr, 0, value.dump().data());
  }
}

Response Service::DeleteValue(const std::string &cube_name,
                              [[maybe_unused]] const ListKeyValues &keys) {
  return delete_handler_(cube_name.data(), nullptr, 0);
}

void Service::init(service::CubeFactory *factory, const std::string &log_file) {
  return init_handler_(factory, log_file.data());
}

Response Service::ReadHelp() {
  return service_help_();
}

}  // namespace polycube::polycubed::Rest::Resources::Data::Lib
