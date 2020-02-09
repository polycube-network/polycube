#pragma once

#include "chrono"
#include "sstream"

namespace Utils {

template <typename T>
std::string join(const T &v, const std::string &delim) {
  std::ostringstream s;
  for (const auto &i : v) {
    if (&i != &v[0])
      s << delim;
    s << i;
  }
  return s.str();
}

uint64_t genTimestamp() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}
}  // namespace Utils
