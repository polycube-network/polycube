/*
 * Copyright 2017 The Polycube Authors
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

#include <memory>
#include <string>
#include <vector>

#include "polycube/common.h"

namespace polycube {
namespace service {

// class Cube;

class RawTable {
  // friend class Cube;

 public:
  RawTable();
  ~RawTable();

  void get(const void *key, void *value);
  void set(const void *key, const void *value);
  void remove(const void *key);

  int first(void *key);
  int next(const void *key, void *next);

  // protected:
  RawTable(void *op);

 private:
  class impl;
  std::shared_ptr<impl> pimpl_;  // TODO: how to use unique_ptr?
};

template <class ValueType>
class ArrayTable : protected RawTable {
  // friend class Cube;

 public:
  ArrayTable() : RawTable(){};
  ~ArrayTable(){};

  ValueType get(const uint32_t &key) {
    ValueType t;
    RawTable::get(&key, &t);
    return t;
  };

  std::vector<std::pair<uint32_t, ValueType>> get_all() {
    std::vector<std::pair<uint32_t, ValueType>> ret;
    uint32_t key;

    if (RawTable::first(&key)) {
      return ret;
    }

    do {
      ret.emplace_back(key, get(key));
    } while (!RawTable::next(&key, &key));

    return ret;
  }

  void set(const uint32_t &key, const ValueType &value) {
    RawTable::set(&key, &value);
  }

  // private:
  ArrayTable(void *op) : RawTable(op){};
};

template <class ValueType>
class PercpuArrayTable : protected RawTable {
  // friend class Cube;

 public:
  PercpuArrayTable() : RawTable(), ncpus_(get_possible_cpu_count()){};
  ~PercpuArrayTable(){};

  std::vector<ValueType> get(const uint32_t &key) {
    std::vector<ValueType> t(ncpus_);
    RawTable::get(&key, t.data());
    return t;
  };

  std::vector<std::pair<uint32_t, std::vector<ValueType>>> get_all() {
    std::vector<std::pair<uint32_t, std::vector<ValueType>>> ret;
    uint32_t key;

    if (RawTable::first(&key)) {
      return ret;
    }

    do {
      ret.emplace_back(key, get(key));
    } while (!RawTable::next(&key, &key));

    return ret;
  }

  void set(const uint32_t &key, const std::vector<ValueType> &value) {
    RawTable::set(&key, value.data());
  }

  // sets the same value on all cpus
  void set(const uint32_t &key, const ValueType &value) {
    std::vector<ValueType> v(ncpus_, value);
    set(key, v);
  }

  // private:
  PercpuArrayTable(void *op) : RawTable(op), ncpus_(get_possible_cpu_count()){};
  unsigned int ncpus_;
};

template <class KeyType, class ValueType>
class HashTable : protected RawTable {
  // friend class Cube;

 public:
  HashTable() : RawTable(){};
  ~HashTable(){};

  ValueType get(const KeyType &key) {
    ValueType t;
    RawTable::get(&key, &t);
    return t;
  }

  std::vector<std::pair<KeyType, ValueType>> get_all() {
    std::vector<std::pair<KeyType, ValueType>> ret;
    KeyType key;

    if (RawTable::first(&key)) {
      return ret;
    }

    do {
      ret.emplace_back(key, get(key));
    } while (!RawTable::next(&key, &key));

    return ret;
  }

  void set(const KeyType &key, const ValueType &value) {
    RawTable::set(&key, &value);
  }

  void remove(const KeyType &key) {
    RawTable::remove(&key);
  }

  void remove_all() {
    KeyType k;
    while (!RawTable::first(&k)) {
      remove(k);
    }
  }
  // private:
  HashTable(void *op) : RawTable(op){};
};

template <class KeyType, class ValueType>
class PercpuHashTable : protected RawTable {
  // friend class Cube;
 public:
  PercpuHashTable() : RawTable(), ncpus_(get_possible_cpu_count()){};
  ~PercpuHashTable(){};

  std::vector<ValueType> get(const KeyType &key) {
    std::vector<ValueType> t(ncpus_);
    RawTable::get(&key, t.data());
    return t;
  }

  std::vector<std::pair<KeyType, std::vector<ValueType>>> get_all() {
    std::vector<std::pair<KeyType, std::vector<ValueType>>> ret;
    KeyType key;

    if (RawTable::first(&key)) {
      return ret;
    }

    do {
      ret.emplace_back(key, get(key));
    } while (!RawTable::next(&key, &key));

    return ret;
  }

  void set(const KeyType &key, const std::vector<ValueType> &value) {
    RawTable::set(&key, value.data());
  }

  // sets the same value on all cpus
  void set(const KeyType &key, const ValueType &value) {
    std::vector<ValueType> v(ncpus_, value);
    set(key, v);
  }

  void remove(const KeyType &key) {
    RawTable::remove(&key);
  }

  void remove_all() {
    KeyType k;
    while (!RawTable::first(&k)) {
      remove(k);
    }
  }

  // private:
  PercpuHashTable(void *op) : RawTable(op), ncpus_(get_possible_cpu_count()){};
  unsigned int ncpus_;
};

}  // namespace service
}  // namespace polycube
