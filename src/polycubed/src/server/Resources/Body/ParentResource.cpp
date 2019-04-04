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
#include "ParentResource.h"

#include <algorithm>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ChoiceResource.h"
#include "JsonNodeField.h"
#include "ListResource.h"

namespace polycube::polycubed::Rest::Resources::Body {

ParentResource::ParentResource(const std::string &name,
                               const std::string &description,
                               const std::string &cli_example,
                               const Body::ParentResource *parent,
                               PolycubedCore *core,
                               const std::vector<JsonNodeField> &node_fields,
                               bool configuration, bool init_only_config,
                               bool container_presence, bool rpc_action)
    : Resource(name, description, cli_example, parent, core, configuration,
               init_only_config, node_fields),
      children_{},
      container_presence_(container_presence),
      rpc_action_(rpc_action) {}

std::vector<Response> ParentResource::BodyValidate(const std::string &cube_name,
                                                   const ListKeyValues &keys,
                                                   nlohmann::json &body,
                                                   bool initialization) const {
  std::vector<Response> errors;

  for (const auto &node_field : node_fields_) {
    const auto node_validation = node_field.Validate(this, cube_name, keys);
    if (node_validation != kOk) {
      errors.push_back({node_validation, ::strdup(name_.data())});
      return errors;
    }
  }

  std::vector<std::shared_ptr<Resource>> choices;
  auto body_copy = body;
  for (auto &child : children_) {
    const auto &child_name = child->Name();

    // Choices are a special case
    if (std::dynamic_pointer_cast<ChoiceResource>(child) != nullptr) {
      choices.push_back(child);
    } else {
      // if children is not present in request body
      if (body_copy.count(child_name) == 0) {
        // but it is required (both mandatory and method = POST or PUT
        if (initialization && child->IsMandatory()) {
          errors.push_back(
              {ErrorTag::kMissingAttribute, ::strdup(child_name.data())});
        }
      } else {
        // non configuration nodes are read only;
        // same applies for configuration nodes marked as init-only-config
        // during an operation different from initialization
        if (!child->IsConfiguration() ||
            (!initialization && child->IsInitOnlyConfig())) {
          errors.push_back(
              {ErrorTag::kInvalidValue, ::strdup(child_name.data())});
        } else {
          std::vector<Response> child_errors;
          // if the child is a list, perform the check over all elements
          if (auto list = std::dynamic_pointer_cast<ListResource>(child)) {
            child_errors = list->BodyValidateMultiple(
                cube_name, keys, body_copy[child_name], initialization);
          } else {
            child_errors = child->BodyValidate(
                cube_name, keys, body_copy[child_name], initialization);
          }

          errors.reserve(errors.size() + child_errors.size());

          // remove current parsed element from the body so that in the
          // eventuality of choices, they will operate only on unparsed
          // elements. moreover, this is required for detecting unparsed
          // elements, that may be typos
          body_copy.erase(child->Name());
          std::move(std::begin(child_errors), std::end(child_errors),
                    std::back_inserter(errors));
        }
      }
    }
  }

  for (const auto &choice : choices) {
    // notice: body_copy is stripped of elements correctly parsed
    // by the current choice (if any)
    auto choice_errors =
        choice->BodyValidate(cube_name, keys, body_copy, initialization);
    // the method will return a single UnparsableChoice if it was not
    // mandatory and it could not match any case.
    if (choice_errors[0].error_tag != ErrorTag::kUnparsableChoice) {
      errors.reserve(errors.size() + choice_errors.size());
      std::move(std::begin(choice_errors), std::end(choice_errors),
                std::back_inserter(errors));
    }
  }

  errors.reserve(errors.size() + body_copy.size());
  // body_copy should be empty as all items must be correctly parsed. If
  // it is not, then there is an error in the input data
  for (auto &unparsed : body_copy.items()) {
    errors.push_back(
        {ErrorTag::kInvalidValue, ::strdup(unparsed.key().data())});
  }

  return errors;
}

void ParentResource::AddChild(std::shared_ptr<Resource> &&child) {
  children_.push_back(std::move(child));
}

bool ParentResource::IsMandatory() const {
  if (container_presence_)
    return true;
  if (rpc_action_)
    return false;
  return std::end(children_) !=
         std::find_if(std::begin(children_), std::end(children_),
                      [](const std::shared_ptr<Resource> &child) {
                        return child->IsMandatory();
                      });
}

bool ParentResource::IsConfiguration() const {
  return configuration_ ||
         std::end(children_) !=
             std::find_if(std::begin(children_), std::end(children_),
                          [](const std::shared_ptr<Resource> &child) {
                            return child->IsConfiguration();
                          });
}

void ParentResource::SetDefaultIfMissing(nlohmann::json &body,
                                         bool initialization) const {
  for (const auto &child : children_) {
    auto &child_body = body[child->Name()];
    // Lists can't be defaulted. The only possible this is if a list
    // (as json array) is provided in request body. In this situation
    // each element of the array can be defaulted.
    if (std::dynamic_pointer_cast<ListResource>(child) != nullptr) {
      if (!child_body.empty()) {
        for (auto &entry : child_body) {
          child->SetDefaultIfMissing(entry, initialization);
        }
      }
    } else {
      // Set default only for configuration nodes. During initialization
      // all can be defaulted, otherwise only the ones that are not marked
      // as init-only-config
      if (child->IsConfiguration() &&
          (initialization || !child->IsInitOnlyConfig())) {
        child->SetDefaultIfMissing(child_body, initialization);
      }
    }
    if (child_body.empty()) {
      body.erase(child->Name());
    }
  }
}

std::shared_ptr<Resource> ParentResource::Child(
    const std::string &child_name) const {
  auto child = std::find_if(
      std::begin(children_), std::end(children_),
      [=](std::shared_ptr<Resource> r) { return r->Name() == child_name; });
  return (child == std::end(children_)) ? nullptr : *child;
}

nlohmann::json ParentResource::ToHelpJson() const {
  nlohmann::json val = Resource::ToHelpJson();

  if (val.count("type") == 0) {
    if (rpc_action_) {
      val["type"] = "action";
    } else {
      val["type"] = "complex";
    }
  }

  return val;
}

}  // namespace polycube::polycubed::Rest::Resources::Body
