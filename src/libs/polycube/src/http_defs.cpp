/*
 * This file is a reduced version of the original present at
 * https://github.com/oktal/pistache/.
 */

/* http_defs.cc
   Mathieu Stefani, 01 September 2015

   Implementation of http definitions
*/

#include "polycube/services/http_defs.h"

namespace polycube {
namespace service {

namespace Http {

const char *versionString(Version version) {
    switch (version) {
        case Version::Http10:
            return "HTTP/1.0";
        case Version::Http11:
            return "HTTP/1.1";
    }
}

const char* methodString(Method method)
{
    switch (method) {
#define METHOD(name, str) \
    case Method::name: \
        return str;
    HTTP_METHODS
#undef METHOD
    }
}

Method stringMethod(const std::string &method) {
  std::string strings[] = {
#define METHOD(_, m) m,
    HTTP_METHODS
#undef METHOD
  };

  for (int i = 0; i < sizeof(strings)/sizeof(strings[0]); i++) {
    if (strings[i] == method)
      return static_cast<Method>(i);
  }

  // TODO: what is method is non valid?
}

const char* codeString(Code code)
{
    switch (code) {
#define CODE(_, name, str) \
    case Code::name: \
         return str;
    STATUS_CODES
#undef CODE
    }

    return "";
}

std::ostream& operator<<(std::ostream& os, Version version) {
    os << versionString(version);
    return os;
}

std::ostream& operator<<(std::ostream& os, Method method) {
    os << methodString(method);
    return os;
}

std::ostream& operator<<(std::ostream& os, Code code) {
    os << codeString(code);
    return os;
}

} // namespace Http

}  // namespace service
}  // namespace polycube
