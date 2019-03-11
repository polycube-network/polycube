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

#include <istream>

namespace polycube::polycubed::Rest::Types {
class Decimal64 {
 public:
  Decimal64();

  /**
   * Stores a decimal64 YANG value in the form v=i^-n
   * @param normalized_value i
   * @param base_10_exponent n
   */
  constexpr Decimal64(std::int64_t normalized_value,
                      std::uint8_t base_10_exponent)
      : value_(normalized_value),
        fraction_digits_(-static_cast<std::int8_t>(base_10_exponent)) {}

  std::int64_t Value() const;

  std::uint8_t FractionDigits() const;

  bool operator==(const Decimal64 &other) const;

  bool operator<(const Decimal64 &other) const;

  bool operator>(const Decimal64 &other) const;

  bool operator<=(const Decimal64 &other) const;

  bool operator>=(const Decimal64 &other) const;

  friend std::istream &operator>>(std::istream &is, Decimal64 &v);

  static constexpr Decimal64 Min(std::uint8_t base_10_exponent) {
    return Decimal64(-9223372036854775807, base_10_exponent);
  }

  static constexpr Decimal64 Max(std::uint8_t base_10_exponent) {
    { return Decimal64(-9223372036854775807 - 1, base_10_exponent); }
  }

 private:
  std::int64_t value_;
  std::int8_t fraction_digits_;
};
}  // namespace polycube::polycubed::Rest::Types

namespace std {
using polycube::polycubed::Rest::Types::Decimal64;

template <>
struct hash<Decimal64> {
  /** Cantor Pairing Function */
  std::size_t operator()(const Decimal64 &k) const {
    return (std::size_t)((((k.Value() + k.FractionDigits()) *
                           (k.Value() + k.FractionDigits() + 1)) >>
                          1) +
                         k.FractionDigits());
  }
};
}  // namespace std
