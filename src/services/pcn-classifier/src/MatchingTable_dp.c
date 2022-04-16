/*
 * Copyright 2020 The Polycube Authors
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


// Template parameters:
// PREFIX_MATCHER whether matching is based on lpm trie (1) or hash map (0)
// FIELD          packet field to perform the match on
// TYPE           c type of the matching field
// CLASSES_COUNT  number of traffic classes
// WILDCARD       whether there is a wildcard to match on (1 true, 0 false)


#if _PREFIX_MATCHER
struct _FIELD_lpm_key {
  u32 prefix_len;
  _TYPE key;
};
BPF_LPM_TRIE(_FIELD_rules, struct _FIELD_lpm_key, struct bitvector,
             _CLASSES_COUNT);

#else

// Size of rules table is _CLASSES_COUNT + 1 to handle cases of l4matcher where
// both tcp and udp are set by a single traffic class (e.g. a class with
// dport set but not l4proto)
// Wildcard bitvector is stored into an array map to avoid exceeding the maximum
// stack size of 512 bytes
BPF_HASH(_FIELD_rules, _TYPE, struct bitvector, _CLASSES_COUNT + 1);
#if _WILDCARD
BPF_ARRAY(_FIELD_wildcard_bv, struct bitvector, 1);
#endif

#endif