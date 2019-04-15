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
#include "lexical_cast.h"

#include <algorithm>
#include <string>

namespace polycube::polycubed::Rest::Types {
template <>
bool lexical_cast<bool>(const std::string &value) {
  try {
    std::string lower;
    lower.reserve(value.length());
    std::transform(std::begin(value), std::end(value),
                   std::back_inserter(lower),
                   static_cast<int (*)(int)>(std::tolower));
    if (lower == "false") {
      return false;
    } else if (lower == "true") {
      return true;
    } else {
      throw bad_lexical_cast();
    }
  } catch (...) {
    throw bad_lexical_cast();
  }
}

template <>
int8_t lexical_cast<int8_t>(const std::string &value) {
  try {
    auto val = std::stoll(value);
    if (val > std::numeric_limits<int8_t>::max() ||
        val < std::numeric_limits<int8_t>::min()) {
      throw bad_lexical_cast();
    }
    return val;
  } catch (...) {
    throw bad_lexical_cast();
  }
}

template <>
uint8_t lexical_cast<uint8_t>(const std::string &value) {
  try {
    auto val = std::stoull(value);
    if (val > std::numeric_limits<uint8_t>::max()) {
      throw bad_lexical_cast();
    }
    return val;
  } catch (...) {
    throw bad_lexical_cast();
  }
}

template <>
int16_t lexical_cast<int16_t>(const std::string &value) {
  try {
    auto val = std::stoll(value);
    if (val > std::numeric_limits<int16_t>::max() ||
        val < std::numeric_limits<int16_t>::min()) {
      throw bad_lexical_cast();
    }
    return val;
  } catch (...) {
    throw bad_lexical_cast();
  }
}

template <>
uint16_t lexical_cast<uint16_t>(const std::string &value) {
  try {
    auto val = std::stoull(value);
    if (val > std::numeric_limits<uint16_t>::max()) {
      throw bad_lexical_cast();
    }
    return val;
  } catch (...) {
    throw bad_lexical_cast();
  }
}

template <>
int32_t lexical_cast<int32_t>(const std::string &value) {
  try {
    auto val = std::stoll(value);
    if (val > std::numeric_limits<int32_t>::max() ||
        val < std::numeric_limits<int32_t>::min()) {
      throw bad_lexical_cast();
    }
    return val;
  } catch (...) {
    throw bad_lexical_cast();
  }
}

template <>
uint32_t lexical_cast<uint32_t>(const std::string &value) {
  try {
    auto val = std::stoull(value);
    if (val > std::numeric_limits<uint32_t>::max()) {
      throw bad_lexical_cast();
    }
    return val;
  } catch (...) {
    throw bad_lexical_cast();
  }
}

template <>
int64_t lexical_cast<int64_t>(const std::string &value) {
  try {
    auto val = std::stoll(value);
    if (val > std::numeric_limits<int64_t>::max() ||
        val < std::numeric_limits<int64_t>::min()) {
      throw bad_lexical_cast();
    }
    return val;
  } catch (...) {
    throw bad_lexical_cast();
  }
}

template <>
uint64_t lexical_cast<uint64_t>(const std::string &value) {
  try {
    auto val = std::stoull(value);
    if (val > std::numeric_limits<uint64_t>::max()) {
      throw bad_lexical_cast();
    }
    return val;
  } catch (...) {
    throw bad_lexical_cast();
  }
}

template <>
float lexical_cast<float>(const std::string &value) {
  try {
    return std::stof(value);
  } catch (...) {
    throw bad_lexical_cast();
  }
}

template <>
double lexical_cast<double>(const std::string &value) {
  try {
    return std::stod(value);
  } catch (...) {
    throw bad_lexical_cast();
  }
}

template <>
Decimal64 lexical_cast<Decimal64>(const std::string &value) {
  try {
    std::size_t idx;
    auto numerator = std::stoll(value, &idx);

    if (idx == value.length()) {
      return Decimal64{numerator, 0};
    }

    if (value.at(idx) != '.') {
      throw bad_lexical_cast();
    }

    idx += 1;
    if (idx == value.length()) {
      return Decimal64{numerator, 0};
    }

    auto denominator_str = value.substr(idx);
    std::uint8_t fraction_digits = denominator_str.length();
    if (fraction_digits > 18) {
      throw bad_lexical_cast();
    }

    auto denominator = std::stoll(denominator_str, &idx);
    if (idx != fraction_digits) {
      throw bad_lexical_cast();
    }
    unsigned shift = 1;
    for (std::size_t i = 0; i < fraction_digits; ++i) {
      shift *= 10;
    }

    auto parsed = ((numerator * shift) + denominator);
    return Decimal64{parsed, fraction_digits};
  } catch (...) {
    throw bad_lexical_cast();
  }
}
}