/**
 *  Copyright to polfosol
 *  https://raw.githubusercontent.com/gaspardpetit/base64/master/src/polfosol/polfosol.h
 */
#include "Base64.h"
#include <string>

namespace polycube::polycubed::Rest::Server {

std::string Base64::decode(const std::string &str64) {
  return decode(str64.c_str(), str64.size());
}

std::string Base64::decode(const void *data, const size_t len) {
  auto p = (unsigned char *)data;
  int pad = len > 0 && (len % 4 || p[len - 1] == '=');
  const size_t L = ((len + 3) / 4 - pad) * 4;
  std::string str(L / 4 * 3 + pad, '\0');

  for (size_t i = 0, j = 0; i < L; i += 4) {
    int n = B64index[p[i]] << 18 | B64index[p[i + 1]] << 12 |
            B64index[p[i + 2]] << 6 | B64index[p[i + 3]];
    str[j++] = n >> 16;
    str[j++] = n >> 8 & 0xFF;
    str[j++] = n & 0xFF;
  }
  if (pad) {
    int n = B64index[p[L]] << 18 | B64index[p[L + 1]] << 12;
    str[str.size() - 1] = n >> 16;

    if (len > L + 2 && p[L + 2] != '=') {
      n |= B64index[p[L + 2]] << 6;
      str.push_back(n >> 8 & 0xFF);
    }
  }
  return str;
}
}  // namespace polycube::polycubed::Rest::Server
