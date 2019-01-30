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
#include "ResponseGenerator.h"

#include <vector>

#include "polycube/services/json.hpp"

namespace polycube::polycubed::Rest::Server {
namespace {
using nlohmann::json;
auto error = R"({
  "error-type": "application",
  "error-tag": ""
})"_json;

auto errors = R"({
  "ietf-restconf:errors": {
    "error": []
  }
})"_json;
}  // namespace

void ResponseGenerator::Generate(std::vector<Response> &&response,
                                 Pistache::Http::ResponseWriter &&writer) {
  using Pistache::Http::Code;
  auto mime = Pistache::Http::Mime::MediaType(
      "application/yang.data+json; charset=utf-8");
  writer.headers().add<Pistache::Http::Header::ContentType>(mime);
  for (auto &error : response) {
    if (error.message == nullptr) {
      error.message = strdup("");
    }
  }
  if (response[0].error_tag == kOk) {
    writer.send(Code::Ok, response[0].message, mime);
  } else if (response[0].error_tag == kCreated) {
    writer.send(Code::Created, response[0].message, mime);
  } else if (response[0].error_tag == kNoContent) {
    writer.send(Code::No_Content);
  } else if (response[0].error_tag == kBadRequest) {
    writer.send(Code::Bad_Request, response[0].message);
  } else if (response[0].error_tag == kGenericError) {
    writer.send(Code::Internal_Server_Error, response[0].message);
  } else {
    auto body = errors;
    Code response_code = Code::Ok;
    for (const auto &err : response) {
      auto single = error;
      switch (err.error_tag) {
      case kInvalidValue:
        response_code = Code::Bad_Request;
        single["error-tag"] = "invalid-value";
        break;
      case kMissingAttribute:
        response_code = Code::Bad_Request;
        single["error-tag"] = "missing-attribute";
        break;
      case kMissingElement:
        response_code = Code::Bad_Request;
        single["error-tag"] = "missing-element";
        break;
      case kBadAttribute:
        response_code = Code::Bad_Request;
        single["error-tag"] = "bad-attribute";
        break;
      case kBadElement:
        response_code = Code::Bad_Request;
        single["error-tag"] = "bad-element";
        break;
      case kDataExists:
        response_code = Code::Conflict;
        single["error-tag"] = "data-exists";
        break;
      case kDataMissing:
        response_code = Code::Conflict;
        single["error-tag"] = "data-missing";
        break;
      case kOperationNotSupported:
        response_code = Code::Method_Not_Allowed;
        single["error-tag"] = "operation-not-supported";
        break;
      default:
        break;
      }
      if (std::strlen(err.message))
        single["error-info"] = err.message;

      body["ietf-restconf:errors"]["error"].push_back(single);
    }
    writer.send(response_code, body.dump(), mime);
  }
  for (auto &error : response) {
    ::free(error.message);
  }
}
}  // namespace polycube::polycubed::Rest::Server
