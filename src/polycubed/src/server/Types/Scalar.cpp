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
#include "Scalar.h"

namespace polycube::polycubed::Rest::Types {

Scalar ScalarFromYang(LY_DATA_TYPE type) {
  switch (type) {
  case LY_TYPE_BOOL:
    return Scalar::Boolean;
  case LY_TYPE_DEC64:
    return Scalar::Decimal;
  case LY_TYPE_EMPTY:
    return Scalar::Empty;
  case LY_TYPE_BINARY:
  case LY_TYPE_BITS:
  case LY_TYPE_ENUM:
  case LY_TYPE_IDENT:
  case LY_TYPE_INST:
  case LY_TYPE_STRING:
  case LY_TYPE_UNION:
    return Scalar::String;
  case LY_TYPE_INT8:
  case LY_TYPE_INT16:
  case LY_TYPE_INT32:
  case LY_TYPE_INT64:
    return Scalar::Integer;
  case LY_TYPE_UINT8:
  case LY_TYPE_UINT16:
  case LY_TYPE_UINT32:
  case LY_TYPE_UINT64:
    return Scalar::Unsigned;
  }
}

std::string ScalarToString(Scalar value) {
  switch (value) {
  case Scalar::Integer:
    return "integer";
  case Scalar::Unsigned:
    return "unsigned";
  case Scalar::Decimal:
    return "decimal";
  case Scalar::String:
    return "string";
  case Scalar::Boolean:
    return "boolean";
  case Scalar::Empty:
    return "empty";
  default:
    return "unknown";
  }
}

}  // namespace polycube::polycubed::Rest::Types
