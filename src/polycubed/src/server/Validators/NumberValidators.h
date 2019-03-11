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
#include <unordered_map>
#include "../Types/Decimal64.h"
#include "ValueValidator.h"

namespace polycube::polycubed::Rest::Validators {
template <typename T>
class NumberValidator : public ValueValidator {
 public:
  void AddRange(T lower_bound, T upper_bound);

  void AddExact(T exact);

  void AddRanges(std::unordered_map<T, T> ranges);

  bool Validate(const std::string &value) const override;

  NumberValidator(T lower_bound, T upper_bound);

 protected:
  bool Validate(T parsed) const;

  std::unordered_map<T, T> ranges_;
};

using Types::Decimal64;
class DecimalValidator : public NumberValidator<Decimal64> {
 public:
  explicit DecimalValidator(std::uint8_t fraction_digits);

  DecimalValidator(std::uint8_t fraction_digits, const Decimal64 &lower_bound,
                   const Decimal64 &upper_bound);

  bool Validate(const std::string &value) const final;

 private:
  std::uint8_t fraction_digits_;
};

template class NumberValidator<std::int8_t>;

template class NumberValidator<std::uint8_t>;

template class NumberValidator<std::int16_t>;

template class NumberValidator<std::uint16_t>;

template class NumberValidator<std::int32_t>;

template class NumberValidator<std::uint32_t>;

template class NumberValidator<std::int64_t>;

template class NumberValidator<std::uint64_t>;

template class NumberValidator<Decimal64>;
}  // namespace polycube::polycubed::Rest::Validators
