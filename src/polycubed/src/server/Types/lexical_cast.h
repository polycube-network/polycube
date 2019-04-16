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

#include <string>
#include "Decimal64.h"

namespace polycube::polycubed::Rest::Types {

struct bad_lexical_cast : public std::exception {};

template <class T>
T lexical_cast(const std::string &value) = delete;

template <>
bool lexical_cast<bool>(const std::string &value);

template <>
int8_t lexical_cast<int8_t>(const std::string &value);

template <>
uint8_t lexical_cast<uint8_t>(const std::string &value);

template <>
int16_t lexical_cast<int16_t>(const std::string &value);

template <>
uint16_t lexical_cast<uint16_t>(const std::string &value);

template <>
int32_t lexical_cast<int32_t>(const std::string &value);

template <>
uint32_t lexical_cast<uint32_t>(const std::string &value);

template <>
int64_t lexical_cast<int64_t>(const std::string &value);

template <>
uint64_t lexical_cast<uint64_t>(const std::string &value);

template <>
float lexical_cast<float>(const std::string &value);

template <>
double lexical_cast<double>(const std::string &value);

template <>
Decimal64 lexical_cast<Decimal64>(const std::string &value);

}  // namespace polycube::polycubed::Rest::Types