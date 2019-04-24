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
#include "ChoiceResource.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../../Validators/InSetValidator.h"
#include "CaseResource.h"
#include "JsonNodeField.h"

namespace polycube::polycubed::Rest::Resources::Body {
ChoiceResource::ChoiceResource(
    const std::string &name, const std::string &description,
    const std::string &cli_example, const Body::ParentResource *parent,
    PolycubedCore *core, bool mandatory,
    std::unique_ptr<const std::string> &&default_case)
    : ParentResource(name, description, cli_example, parent, core,
                     std::vector<JsonNodeField>{}, false, false, false),
      mandatory_(mandatory),
      default_case_(std::move(default_case)) {}

std::vector<Response> ChoiceResource::BodyValidate(const std::string &cube_name,
                                                   const ListKeyValues &keys,
                                                   nlohmann::json &body,
                                                   bool initialization) const {
  bool parsed_child = false;
  std::vector<Response> errors;
  for (const auto &child : children_) {
    const auto &child_name = child->Name();
    std::vector<Response> child_errors;
    // if children is choice or case, pass whole body
    if (std::dynamic_pointer_cast<ChoiceResource>(child) != nullptr ||
        std::dynamic_pointer_cast<CaseResource>(child) != nullptr) {
      child_errors = child->BodyValidate(cube_name, keys, body, initialization);
    } else {  // else pass to the correct child
      if (body.count(child_name) == 0) {
        if (initialization && IsMandatory() && child->IsMandatory()) {
          errors.push_back(
              {ErrorTag::kMissingAttribute, ::strdup(child_name.data())});
        }
      } else {
        if (!child->IsConfiguration() ||
            (!initialization && child->IsInitOnlyConfig())) {
          errors.push_back({ErrorTag::kInvalidValue, nullptr});
        } else {
          child_errors =
              child->BodyValidate(cube_name, keys, body.at(child_name),
                                  initialization && IsMandatory());
          errors.reserve(errors.size() + child_errors.size());
          body.erase(child_name);
          std::move(std::begin(child_errors), std::end(child_errors),
                    std::back_inserter(errors));
        }
      }
    }
    if (child_errors.empty()) {
      if (parsed_child && errors.empty()) {
        errors.push_back({ErrorTag::kBadAttribute, ::strdup(name_.data())});
      } else {
        parsed_child = true;
      }
    }
  }
  if (errors.empty() || !parsed_child) {
    errors.push_back({ErrorTag::kUnparsableChoice, nullptr});
  }
  return errors;
}

bool ChoiceResource::IsMandatory() const {
  return mandatory_ &&
         dynamic_cast<const CaseResource *const>(parent_) != nullptr;
}

void ChoiceResource::SetDefaultIfMissing(nlohmann::json &body) const {
  if (default_case_ != nullptr) {
    auto def = std::find_if(
        std::begin(children_), std::end(children_),
        [this](const auto &child) { return child->Name() == *default_case_; });
    // no check if def is end because it ensured by YANG validation process
    (*def)->SetDefaultIfMissing(body);
  }
}
}  // namespace polycube::polycubed::Rest::Resources::Body
