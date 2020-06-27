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

#include <polycube/common.h>

#include "./../../../polycubed/src/utils/utils.h"

#define QUEUE_STACK_KERNEL_RELEASE "5.0.0"

namespace polycube::service {

// class Cube;
/** TRADITIONAL eBPF MAPS*/

class RawTable {
  // friend class Cube;

 public:
  RawTable() = default;
  ~RawTable();

  void get(const void *key, void *value);
  void set(const void *key, const void *value);
  void remove(const void *key);

  /** These batch operations works if kernel >= 5.7.0. Moreover, not all map types
   *  are supported. Check 5.7 release or more, if map files contains batch operations within
   *  the map supported ones (https://github.com/torvalds/linux/tree/24085f70a6e1b0cb647ec92623284641d8270637/kernel/bpf).
   *  In Dynmon we have tested:
   *  -Arrays      => OK
   *  -HASH        => OK
   *  -QUEUE/STACK => KO
   *  -PERCPU      => KO
   *
   *  in_batch = nullptr means from the beginning (otherwise start from).
   *  out_batch = count means till the end (otherwise till here).
   *  count is the max_entries of the table (or less, pay attention to errors in that case and errno)
   *  */
  int get_batch(void *keys, void *values, unsigned int *count, void *in_batch = nullptr, void *out_batch = nullptr);
  int get_and_delete_batch(void *keys, void *values, unsigned int *count, void *in_batch = nullptr, void *out_batch = nullptr);
  int update_batch(void *keys, void *values, unsigned int *count);

  int first(void *key);
  int next(const void *key, void *next);

  // protected:
  explicit RawTable(void *op);

 private:
  class impl;
  std::shared_ptr<impl> pimpl_;  // TODO: how to use unique_ptr?
};

template <class ValueType>
class ArrayTable : protected RawTable {
  // friend class Cube;

 public:
  ArrayTable() : RawTable(){};
  ~ArrayTable()= default;

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
  explicit ArrayTable(void *op) : RawTable(op){};
};

template <class ValueType>
class PercpuArrayTable : protected RawTable {
  // friend class Cube;

 public:
  PercpuArrayTable() : RawTable(), ncpus_(get_possible_cpu_count()){};
  ~PercpuArrayTable()= default;

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
  explicit PercpuArrayTable(void *op) : RawTable(op), ncpus_(get_possible_cpu_count()){};
  unsigned int ncpus_;
};

template <class KeyType, class ValueType>
class HashTable : protected RawTable {
  // friend class Cube;

 public:
  HashTable() : RawTable(){};
  ~HashTable()= default;

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
  explicit HashTable(void *op) : RawTable(op){};
};

template <class KeyType, class ValueType>
class PercpuHashTable : protected RawTable {
  // friend class Cube;
 public:
  PercpuHashTable() : RawTable(), ncpus_(get_possible_cpu_count()){};
  ~PercpuHashTable()= default;

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
  explicit PercpuHashTable(void *op) : RawTable(op), ncpus_(get_possible_cpu_count()){};
  unsigned int ncpus_;
};

/** QUEUE/STACK MAPS*/

class RawQueueStackTable {

 public:
  RawQueueStackTable() = default;
  ~RawQueueStackTable();

  int pop(void *value);
  int push(const void *value);

  explicit RawQueueStackTable(void *op);

 private:
  class impl;
  std::shared_ptr<impl> pimpl_;
};


template <class ValueType>
class QueueStackTable : protected RawQueueStackTable {

 public:
  QueueStackTable() : RawQueueStackTable(){};
  ~QueueStackTable()= default;

  ValueType pop() {
    ValueType t;
    RawQueueStackTable::pop(&t);
    return t;
  };

  void push(const ValueType &value) {
    RawQueueStackTable::push(&value);
  }

  explicit QueueStackTable(void *op) : RawQueueStackTable(op){};
};

}  // namespace polycube
