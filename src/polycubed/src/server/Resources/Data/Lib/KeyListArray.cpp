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
#include "KeyListArray.h"

namespace polycube::polycubed::Rest::Resources::Data::Lib::KeyListArray {

namespace {
Key Generate(Body::ListType type, const std::string &key_name,
             const std::string &key_value) {
  ElementValue v{};
  switch (type) {
  case Body::ListType::kBool:
    v.boolean = key_value == "true";
    return {key_name.data(), BOOLEAN, v};
  case Body::ListType::kString:
    v.string = key_value.data();
    return {key_name.data(), STRING, v};
  case Body::ListType::kInt8:
    v.int8 = static_cast<std::int8_t>(std::stoi(key_value));
    return {key_name.data(), INT8, v};
  case Body::ListType::kInt16:
    v.int16 = static_cast<std::int16_t>(std::stoi(key_value));
    return {key_name.data(), INT16, v};
  case Body::ListType::kInt32:
    v.int32 = static_cast<std::int32_t>(std::stol(key_value));
    return {key_name.data(), INT32, v};
  case Body::ListType::kInt64:
    v.int64 = static_cast<std::int64_t>(std::stoll(key_value));
    return {key_name.data(), INT64, v};
  case Body::ListType::kUint8:
    v.uint8 = static_cast<std::uint8_t>(std::stoul(key_value));
    return {key_name.data(), UINT8, v};
  case Body::ListType::kUint16:
    v.uint16 = static_cast<std::uint16_t>(std::stoul(key_value));
    return {key_name.data(), UINT16, v};
  case Body::ListType::kUint32:
    v.uint32 = static_cast<std::uint32_t>(std::stoul(key_value));
    return {key_name.data(), UINT32, v};
  case Body::ListType::kUint64:
    v.uint64 = static_cast<std::uint64_t>(std::stoull(key_value));
    return {key_name.data(), UINT64, v};
  case Body::ListType::kDecimal:
    v.string = key_value.data();
    return {key_name.data(), DECIMAL, v};
  }
}
}  // namespace

std::vector<Key> Generate(const ListKeyValues &keys) {
  std::vector<Key> key_params;
  for (const auto &key : keys) {
    key_params.push_back(Generate(key.type, key.name, key.value));
  }
  return key_params;
}
}  // namespace polycube::polycubed::Rest::Resources::Data::Lib::KeyListArray
