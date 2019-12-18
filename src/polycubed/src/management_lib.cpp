/*
 * Copyright 2017 The Polycube Authors
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
#include "management_lib.h"

#include "server/Parser/Yang.h"
#include "server/Resources/Data/BaseModel/ConcreteFactory.h"
#include "server/Resources/Data/Lib/ConcreteFactory.h"
#include "server/Resources/Data/Lib/Service.h"

#include "polycube/services/cube_factory.h"

#include <dlfcn.h>

namespace polycube::polycubed {

ManagementLib::ManagementLib(const std::string &uri,
                             const std::string &base_url,
                             const std::string &name, PolycubedCore *core)
    : uri_(uri), base_url_(base_url), name_(name), core_(core) {}

ServiceMetadata ManagementLib::init(service::CubeFactory *factory,
                                    std::string logfile) {
  auto handle_ =
      std::shared_ptr<void>(::dlopen(uri_.data(), RTLD_NOW), [](void *h) {
        if (h != nullptr)
          ::dlclose(h);
      });

  if (!handle_) {
    throw std::logic_error("Cannot load service implementation " + uri_ + ": " +
                           std::string(::dlerror()));
  }

  ServiceMetadata md;

  md.dataModel = get_lib_info(handle_, "data_model");
  md.pyangGitRepoId = get_lib_info(handle_, "pyang_git");
  md.swaggerCodegenGitRepoId = get_lib_info(handle_, "swagger_codegen_git");

  auto rest_factory =
      std::make_unique<Rest::Resources::Data::Lib::ConcreteFactory>(uri_,
                                                                    core_);
  auto base_model_factory =
      std::make_unique<Rest::Resources::Data::BaseModel::ConcreteFactory>(
          uri_, const_cast<PolycubedCore *>(core_));

  auto parser = Rest::Parser::Yang(md.dataModel, std::move(rest_factory),
                                   std::move(base_model_factory));
  service_ = parser.Parse(base_url_, name_, &md);

  auto service_lib =
      std::dynamic_pointer_cast<Rest::Resources::Data::Lib::Service>(service_);
  service_lib->init(factory, logfile);

  return md;
}

ManagementLib::~ManagementLib() {}

std::string ManagementLib::get_lib_info(std::shared_ptr<void> handle,
                                        const std::string &field) {
  typedef const char *(*handle_type)(void);
  auto *result = (handle_type)::dlsym(handle.get(), field.data());
  if (!result) {
    char *const error = ::dlerror();
    if (error) {
      throw std::domain_error(std::string{error});
    }
  }

  return std::string(result());
}

}  // namespace polycube::polycubed