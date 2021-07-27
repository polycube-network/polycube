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

#include "utils.h"

#include <linux/version.h>
#include <sys/utsname.h>
#include <cstring>
#include <stdexcept>

namespace polycube {
namespace polycubed {
namespace utils {

bool check_kernel_version(const std::string &version) {
  // current version
  unsigned int major, minor, patch;
  // required version
  unsigned int major_r, minor_r, patch_r;

  struct utsname buf;
  if (uname(&buf) == -1) {
    throw std::runtime_error("error getting kernel version: " +
                             std::string(std::strerror(errno)));
  }

  sscanf(buf.release, "%u.%u.%u", &major, &minor, &patch);
  sscanf(version.c_str(), "%u.%u.%u", &major_r, &minor_r, &patch_r);

  return KERNEL_VERSION(major, minor, patch) >=
         KERNEL_VERSION(major_r, minor_r, patch_r);
}

std::map<std::string, std::string> strip_port_peers(json &cubes) {
  std::map<std::string, std::string> peers;

  for (auto &cube : cubes) {
    if (!cube.count("ports")) {
      continue;
    }

    auto &ports = cube.at("ports");

    for (auto &port : ports) {
      if (port.count("peer")) {
        auto cube_name = cube.at("name").get<std::string>();
        auto port_name = port.at("name").get<std::string>();
        peers[cube_name + ":" + port_name] = port.at("peer").get<std::string>();
        port.erase("peer");
      }
    }
  }

  return peers;
}

// Used to replace strings in datapath code
void replaceStrAll(std::string &str, const std::string &from, const std::string &to) {
  if (from.empty())
    return;

  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();  // In case 'to' contains 'from', like replacing
    // 'x' with 'yx'
  }
}

std::map<std::string, json> strip_port_tcubes(json &jcube) {
  std::map<std::string, json> cubes;

  if (!jcube.count("ports")) {
    return cubes;
  }

  auto &jports = jcube.at("ports");

  for (auto &jport : jports) {
    if (!jport.count("tcubes")) {
      continue;
    }

    auto port_name = jport.at("name").get<std::string>();

    json jcubes = json::object();
    jcubes["tcubes"] = jport.at("tcubes");
    cubes[port_name] = jcubes;
    jport.erase("tcubes");
  }

  return cubes;
}

// taken from pistache
inline size_t to_utf8(int code, char *buff) {
  if (code < 0x0080) {
    buff[0] = (code & 0x7F);
    return 1;
  } else if (code < 0x0800) {
    buff[0] = (0xC0 | ((code >> 6) & 0x1F));
    buff[1] = (0x80 | (code & 0x3F));
    return 2;
  } else if (code < 0xD800) {
    buff[0] = (0xE0 | ((code >> 12) & 0xF));
    buff[1] = (0x80 | ((code >> 6) & 0x3F));
    buff[2] = (0x80 | (code & 0x3F));
    return 3;
  } else if (code < 0xE000) {  // D800 - DFFF is invalid...
    return 0;
  } else if (code < 0x10000) {
    buff[0] = (0xE0 | ((code >> 12) & 0xF));
    buff[1] = (0x80 | ((code >> 6) & 0x3F));
    buff[2] = (0x80 | (code & 0x3F));
    return 3;
  } else if (code < 0x110000) {
    buff[0] = (0xF0 | ((code >> 18) & 0x7));
    buff[1] = (0x80 | ((code >> 12) & 0x3F));
    buff[2] = (0x80 | ((code >> 6) & 0x3F));
    buff[3] = (0x80 | (code & 0x3F));
    return 4;
  }

  // NOTREACHED
  return 0;
}

inline bool is_hex(char c, int &v) {
  if (0x20 <= c && isdigit(c)) {
    v = c - '0';
    return true;
  } else if ('A' <= c && c <= 'F') {
    v = c - 'A' + 10;
    return true;
  } else if ('a' <= c && c <= 'f') {
    v = c - 'a' + 10;
    return true;
  }
  return false;
}

inline bool from_hex_to_i(const std::string &s, int i, int cnt, int &val) {
  val = 0;
  for (; cnt; i++, cnt--) {
    if (!s[i]) {
      return false;
    }
    int v = 0;
    if (is_hex(s[i], v)) {
      val = val * 16 + v;
    } else {
      return false;
    }
  }
  return true;
}

std::string decode_url(const std::string &s) {
  std::string result;

  for (int i = 0; s[i]; i++) {
    if (s[i] == '%') {
      if (s[i + 1] && s[i + 1] == 'u') {
        int val = 0;
        if (from_hex_to_i(s, i + 2, 4, val)) {
          // 4 digits Unicode codes
          char buff[4];
          size_t len = to_utf8(val, buff);
          if (len > 0) {
            result.append(buff, len);
          }
          i += 5;  // 'u0000'
        } else {
          result += s[i];
        }
      } else {
        int val = 0;
        if (from_hex_to_i(s, i + 1, 2, val)) {
          // 2 digits hex codes
          result += val;
          i += 2;  // '00'
        } else {
          result += s[i];
        }
      }
    } else if (s[i] == '+') {
      result += ' ';
    } else {
      result += s[i];
    }
  }

  return result;
}

}  // namespace utils
}  // namespace polycubed
}  // namespace polycube
