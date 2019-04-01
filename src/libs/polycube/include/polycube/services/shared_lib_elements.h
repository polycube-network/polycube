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

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif
enum ElementType {
  BOOLEAN,
  STRING,
  INT8,
  INT16,
  INT32,
  INT64,
  UINT8,
  UINT16,
  UINT32,
  UINT64,
  DECIMAL
};

union ElementValue {
  bool boolean;
  const char *string;
  int8_t int8;
  int16_t int16;
  int32_t int32;
  int64_t int64;
  uint8_t uint8;
  uint16_t uint16;
  uint32_t uint32;
  uint64_t uint64;
};

typedef struct {
  const char *name;
  enum ElementType type;
  union ElementValue value;
} Key;
#ifdef __cplusplus
}
#endif