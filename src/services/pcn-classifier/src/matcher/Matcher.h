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


#pragma once


#include "../Utils.h"
#include "MatcherInterface.h"
#include "Matcher_dp.h"
#include "MatchingTable_dp.h"


template <class T>
class Matcher : public MatcherInterface {
 public:
  Matcher(Classifier &classifier, MatchingField field)
      : classifier_(classifier),
        field_(field),
        matching_code_template_(matcher_code),
        table_code_template_(matching_table_code),
        bitvector_size_(0) {
    replaceAll(table_code_template_, "_FIELD", FIELD_NAME(field_));
    replaceAll(table_code_template_, "_TYPE", FIELD_TYPE(field_));
    replaceAll(matching_code_template_, "_FIELD", FIELD_NAME(field_));
    replaceAll(matching_code_template_, "_TYPE", FIELD_TYPE(field_));
  };

  MatchingField getField() {
    return field_;
  }

  virtual void initBitvector(uint32_t size) {
    bitvector_size_ = size;
    current_bit_ = 0;
  }

  std::string getTableCode() {
    std::string table_code = table_code_template_;
    replaceAll(table_code, "_CLASSES_COUNT", std::to_string(bitvector_size_));

    if (hasWildcard()) {
      replaceAll(table_code, "_WILDCARD", "1");

    } else {
      replaceAll(table_code, "_WILDCARD", "0");
    }

    return table_code;
  }

  std::string getMatchingCode() {
    std::string matching_code = matching_code_template_;

    replaceAll(matching_code, "_CLASSES_COUNT",
               std::to_string(bitvector_size_));
    replaceAll(matching_code, "_SUBVECT_COUNT",
               std::to_string((bitvector_size_ - 1) / 64 + 1));

    if (hasWildcard()) {
      replaceAll(matching_code, "_WILDCARD", "1");

    } else {
      replaceAll(matching_code, "_WILDCARD", "0");
    }

    return matching_code;
  }

 protected:
  Classifier &classifier_;
  MatchingField field_;
  uint32_t bitvector_size_;
  std::string table_code_template_;
  std::string matching_code_template_;
  uint32_t current_bit_;
  std::vector<uint64_t> wildcard_bitvector_;

  bool hasWildcard() {
    for (auto &it : wildcard_bitvector_) {
      if (it != 0) {
        return true;
      }
    }

    return false;
  }
};