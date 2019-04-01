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

#ifdef __cplusplus
extern "C" {
#endif
enum ErrorTag {
  /** No error (200).
   * Message will contain the response body. */
  kOk,
  /** No error and content created (201). */
  kCreated,
  /** No error and no content (204). */
  kNoContent,
  /** Generic bad request (404). */
  kBadRequest,
  /** Generic error (500). */
  kGenericError,
  /** Generic invalid-value error, no message provided */
  kInvalidValue,
  /** Required body field missing.
   * Message will contain the name of the missing field. */
  kMissingAttribute,
  /** Required path param missing.
   * Message will contain the name of the missing path param. */
  kMissingElement,
  /** A body field value is not correct; e.g., wrong type, out of range,
   * pattern mismatch.
   * Message will contain the name of the malformed field. */
  kBadAttribute,
  /** A path param value is not correct; e.g., wrong type, out of range,
   * pattern mismatch.
   * Message will contain the name of the malformed path param. */
  kBadElement,
  /** Request could not be completed because the relevant data model content
   * already exists.
   * For example, a "create" operation was attempted on data
   * that already exists. */
  kDataExists,
  /** Request could not be completed because the relevant data model content
   * does not exist.
   * For example, a "delete" operation was attempted on data
   * that does not exist. */
  kDataMissing,
  /** Wrong HTTP method used. */
  kOperationNotSupported,
  kUnparsableChoice
};

enum HelpType { SHOW, ADD, SET, DEL, NONE, NO_HELP };

typedef struct Response {
  const enum ErrorTag error_tag;
  char *message;
} Response;
#ifdef __cplusplus
}
#endif
