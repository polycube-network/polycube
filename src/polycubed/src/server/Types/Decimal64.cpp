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
#include "Decimal64.h"
namespace polycube::polycubed::Rest::Types {
bool Decimal64::operator==(const Decimal64 &other) const {
  return value_ == other.value_ && fraction_digits_ == other.fraction_digits_;
}

bool Decimal64::operator<(const Decimal64 &other) const {
  return value_ < other.value_;
}

bool Decimal64::operator>(const Decimal64 &other) const {
  return other < *this;
}

bool Decimal64::operator<=(const Decimal64 &other) const {
  return !(*this > other);
}

bool Decimal64::operator>=(const Decimal64 &other) const {
  return !(*this < other);
}

std::istream &operator>>(std::istream &is, Decimal64 &v) {
  std::string value;
  is >> value;
  try {
    std::size_t idx;
    auto numerator = std::stoll(value, &idx);

    if (idx == value.length()) {
      v.fraction_digits_ = 0;
      v.value_ = numerator;
      return is;
    }
    if (value.at(idx) != '.') {
      is.unget();
      is.setstate(std::ios_base::failbit);
      return is;
    }

    idx += 1;
    if (idx == value.length())
      return is;

    auto denominator_str = value.substr(idx);
    std::uint8_t fraction_digits = denominator_str.length();
    if (fraction_digits > 18) {
      is.unget();
      is.setstate(std::ios_base::failbit);
      return is;
    }
    auto denominator = std::stoll(denominator_str, &idx);
    if (idx != fraction_digits) {
      is.unget();
      is.setstate(std::ios_base::failbit);
      return is;
    }
    unsigned shift = 1;
    for (std::size_t i = 0; i < fraction_digits; ++i) {
      shift *= 10;
    }

    auto parsed = ((numerator * shift) + denominator);
    v.value_ = parsed;
    v.fraction_digits_ = fraction_digits;
    return is;
  } catch (std::invalid_argument &) {
    is.unget();
    is.setstate(std::ios_base::failbit);
    return is;
  }
}

Decimal64::Decimal64() : value_(0), fraction_digits_(1) {}

std::int64_t Decimal64::Value() const {
  return value_;
}

std::uint8_t Decimal64::FractionDigits() const {
  return static_cast<std::uint8_t>(-fraction_digits_);
}
}  // namespace polycube::polycubed::Rest::Types
