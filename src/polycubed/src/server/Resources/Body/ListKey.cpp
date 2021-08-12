/*
 * Copyright 2018-2021 The Polycube Authors
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

#include "ListKey.h"

#include <memory>
#include <string>
#include <typeindex>
#include <utility>
#include <vector>
#include <stdexcept>        // g++ v10 requires to explicitly include stdexcept when using exceptions

namespace polycube::polycubed::Rest::Resources::Body {
ListKey::ListKey(
    LY_DATA_TYPE type, std::string name, std::string original_name,
    std::vector<std::shared_ptr<
        polycube::polycubed::Rest::Validators::ValueValidator>> &&validators)
    : type_(FromYang(type)),
      name_{std::move(name)},
      original_name_{std::move(original_name)},
      validators_{std::move(validators)} {}

ListType ListKey::Type() const {
  return type_;
}

const std::string &ListKey::Name() const {
  return name_;
}

const std::string &ListKey::OriginalName() const {
  return original_name_;
}

const std::vector<std::shared_ptr<Validators::ValueValidator>>
    &ListKey::Validators() const {
  return validators_;
}

ListType ListKey::FromYang(LY_DATA_TYPE type) {
  switch (type) {
  case LY_TYPE_BOOL:
    return ListType::kBool;
  case LY_TYPE_BINARY:
  case LY_TYPE_BITS:
  case LY_TYPE_ENUM:
  case LY_TYPE_IDENT:
  case LY_TYPE_INST:
  case LY_TYPE_STRING:
  case LY_TYPE_UNION:
  case LY_TYPE_EMPTY:
    return ListType::kString;
  case LY_TYPE_DEC64:
    return ListType::kDecimal;
  case LY_TYPE_INT8:
    return ListType::kInt8;
  case LY_TYPE_UINT8:
    return ListType::kUint8;
  case LY_TYPE_INT16:
    return ListType::kInt16;
  case LY_TYPE_UINT16:
    return ListType::kUint16;
  case LY_TYPE_INT32:
    return ListType::kInt32;
  case LY_TYPE_UINT32:
    return ListType::kUint32;
  case LY_TYPE_INT64:
    return ListType::kInt64;
  case LY_TYPE_UINT64:
    return ListType::kUint64;
  default:
    throw std::runtime_error("Unrecognized YANG data type");
  }
}
}  // namespace polycube::polycubed::Rest::Resources::Body
