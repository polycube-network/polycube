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


#include "Matcher.h"


template <class T>
class ExactMatcher : public Matcher<T> {
  using Matcher<T>::classifier_;
  using Matcher<T>::field_;
  using Matcher<T>::bitvector_size_;
  using Matcher<T>::table_code_template_;
  using Matcher<T>::matching_code_template_;
  using Matcher<T>::current_bit_;
  using Matcher<T>::wildcard_bitvector_;
  using Matcher<T>::hasWildcard;

 public:
  ExactMatcher(Classifier &classifier, MatchingField field)
      : Matcher<T>(classifier, field) {
    replaceAll(table_code_template_, "_PREFIX_MATCHER", "0");
    replaceAll(matching_code_template_, "_PREFIX_MATCHER", "0");
  }

  void initBitvector(uint32_t size) override {
    Matcher<T>::initBitvector(size);
    bitvectors_.clear();
    wildcard_bitvector_ = std::vector<uint64_t>((bitvector_size_ - 1) / 64 + 1);
  }

  void appendValueBit(T value) {
    uint32_t subvector = current_bit_ / 64;
    uint32_t subbit = current_bit_ % 64;

    if (bitvectors_.count(value) == 0) {
      bitvectors_[value] = wildcard_bitvector_;
    }
    bitvectors_[value][subvector] |= (uint64_t)1 << subbit;

    current_bit_++;
  }

  void appendValuesBit(std::vector<T> values) {
    uint32_t subvector = current_bit_ / 64;
    uint32_t subbit = current_bit_ % 64;

    for (auto value : values) {
      if (bitvectors_.count(value) == 0) {
        bitvectors_[value] = wildcard_bitvector_;
      }
      bitvectors_[value][subvector] |= (uint64_t)1 << subbit;
    }

    current_bit_++;
  }

  void appendWildcardBit() override {
    uint32_t subvector = current_bit_ / 64;
    uint32_t subbit = current_bit_ % 64;

    for (auto &it : bitvectors_) {
      it.second[subvector] |= (uint64_t)1 << subbit;
    }
    wildcard_bitvector_[subvector] |= (uint64_t)1 << subbit;

    current_bit_++;
  }

  void loadTable(int prog_index, ProgramType direction) override {
    std::string table_name = FIELD_NAME(field_) + "_rules"; 
    auto table = classifier_.get_raw_table(table_name, prog_index, direction);
    for (auto &it : bitvectors_) {
      table.set(&it.first, it.second.data());
    }

    if (hasWildcard()) {
      table_name = FIELD_NAME(field_) + "_wildcard_bv";
      table = classifier_.get_raw_table(table_name, prog_index, direction);
      int zero = 0;
      table.set(&zero, wildcard_bitvector_.data());
    }
  }

  bool isActive() override {
    return bitvectors_.size() > 0;
  }

 private:
  std::unordered_map<T, std::vector<uint64_t>> bitvectors_;
};