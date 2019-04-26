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
#include "ListResource.h"

#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../../Validators/ValueValidator.h"
#include "JsonNodeField.h"
#include "ListKey.h"

#include "../../Types/lexical_cast.h"

namespace polycube::polycubed::Rest::Resources::Body {
ListResource::ListResource(const std::string &name,
                           const std::string &description,
                           const std::string &cli_example,
                           const Body::ParentResource *parent,
                           PolycubedCore *core, std::vector<ListKey> &&keys,
                           const std::vector<JsonNodeField> &node_fields,
                           bool configuration, bool init_only_config)
    : ParentResource(name, description, cli_example, parent, core, node_fields,
                     configuration, init_only_config, false),
      keys_{std::move(keys)} {}

bool ListResource::ValidateKeys(
    std::unordered_map<std::string, std::string> keys) const {
  for (const auto &key : keys_) {
    if (keys.count(key.Name()) == 0)
      return false;
    const auto &value = keys.at(key.Name());
    for (const auto &validator : key.Validators()) {
      if (!validator->Validate(value))
        return false;
    }
  }
  return true;
}

const Response ListResource::ReadValue(const std::string &cube_name,
                                       const ListKeyValues &keys) const {
  const auto &parent_value = Resource::ReadValue(cube_name, keys);
  auto jvalue = nlohmann::json::parse(parent_value.message)[name_];
  ListKeyValues current_keys;
  for (const auto &key : keys_) {
    const auto nk = std::find_if(
        std::begin(keys), std::end(keys),
        [=](const ListKeyValue &k) { return k.name == key.Name(); });
    if (nk != std::end(keys)) {
      current_keys.push_back(*nk);
    }
  }
  for (auto &element : jvalue) {
    unsigned matching_keys = 0;
    for (const auto &key : current_keys) {
      if (element[key.name] == key.value) {
        matching_keys += 1;
      }
      if (matching_keys == keys_.size())
        return {ErrorTag::kOk, ::strdup(element.dump().data())};
    }
  }
  throw std::runtime_error(
      "Unreachable since keys are mandatory and validated.");
}

const Response ListResource::ReadWhole(const std::string &cube_name,
                                       const ListKeyValues &keys) const {
  return ParentResource::ReadValue(cube_name, keys);
}

void ListResource::SetDefaultIfMissing(nlohmann::json &body) const {
  // keys default values must be ignored (RFC7950#7.8.2)
  std::set<std::string> key_names;
  for (const auto &key : keys_) {
    key_names.emplace(key.Name());
  }

  for (const auto &child : children_) {
    // Set default only for configuration nodes. During initialization
    // all non-keys can be defaulted, otherwise only the ones that are not
    // marked as init-only-config.
    if (key_names.count(child->Name()) == 0 && child->IsConfiguration() &&
        !child->IsInitOnlyConfig()) {
      child->SetDefaultIfMissing(body[child->Name()]);
      if (body[child->Name()].empty()) {
        body.erase(child->Name());
      }
    }
  }
}

void ListResource::FillKeys(nlohmann::json &body, const ListKeyValues &keys) {
  // TODO: is there a more efficient implementation of this?
  for (auto &key : keys_) {
    for (auto &kv : keys) {
      if (key.Name() == kv.name) {
        // TODO: check if the key is present in the body and compare the data
        switch (key.Type()) {
        case ListType::kBool:
          body[key.OriginalName()] = Types::lexical_cast<bool>(kv.value);
          break;
        case ListType::kInt8:
          body[key.OriginalName()] = Types::lexical_cast<int8_t>(kv.value);
          break;
        case ListType::kInt16:
          body[key.OriginalName()] = Types::lexical_cast<int16_t>(kv.value);
          break;
        case ListType::kInt32:
          body[key.OriginalName()] = Types::lexical_cast<int32_t>(kv.value);
          break;
        case ListType::kInt64:
          body[key.OriginalName()] = Types::lexical_cast<int64_t>(kv.value);
          break;
        case ListType::kUint8:
          body[key.OriginalName()] = Types::lexical_cast<uint8_t>(kv.value);
          break;
        case ListType::kUint16:
          body[key.OriginalName()] = Types::lexical_cast<uint16_t>(kv.value);
          break;
        case ListType::kUint32:
          body[key.OriginalName()] = Types::lexical_cast<uint32_t>(kv.value);
          break;
        case ListType::kUint64:
          body[key.OriginalName()] = Types::lexical_cast<uint64_t>(kv.value);
          break;
        default:
          body[key.OriginalName()] = kv.value;
        }
        break;
      }
    }
  }
}

std::vector<Response> ListResource::BodyValidateMultiple(
    const std::string &cube_name, const ListKeyValues &keys,
    nlohmann::json &body, bool initialization) const {
  std::vector<Response> errors;

  nlohmann::json jbody;
  if (body.empty()) {
    jbody = nlohmann::json::parse("[]");
  } else {
    jbody = body;
  }

  if (!body.is_array()) {
    errors.push_back({ErrorTag::kInvalidValue, nullptr});
    return errors;
  }

  for (auto &elem : jbody) {
    if (initialization) {
      SetDefaultIfMissing(elem);
    }
    auto body = BodyValidate(cube_name, keys, elem, initialization);
    errors.reserve(errors.size() + body.size());
    std::copy(std::begin(body), std::end(body), std::back_inserter(errors));
  }

  return errors;
}

bool ListResource::IsMandatory() const {
  return false;
}

nlohmann::json ListResource::ToHelpJson() const {
  nlohmann::json val = Resource::ToHelpJson();

  if (val.count("type") == 0) {
    val["type"] = "list";
  }

  return val;
}

}  // namespace polycube::polycubed::Rest::Resources::Body
