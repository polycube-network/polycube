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

#include "polycube/services/table.h"

#include <api/BPFTable.h>

using ebpf::BPFTable;

namespace polycube {
namespace service {

// new api
class RawTable::impl {
 public:
  impl(void *op);
  ~impl();

  void get(const void *key, void *value);
  void set(const void *key, const void *value);
  void remove(const void *key);

  int first(void *key);
  int next(const void *key, void *next);

 private:
  int fd_;
};

RawTable::impl::~impl() {}

RawTable::impl::impl(void *op): fd_(*(int*)op) {}

void RawTable::impl::get(const void *key, void *value) {
  if (bpf_lookup_elem(fd_, const_cast<void *>(key),
                      const_cast<void *>(value))) {
    throw std::runtime_error("Table get error: " +
      std::string(std::strerror(errno)));
  }
}

void RawTable::impl::set(const void *key, const void *value) {
  if (bpf_update_elem(fd_, const_cast<void *>(key),
                      const_cast<void *>(value), 0)) {
    throw std::runtime_error("Table set error: " +
      std::string(std::strerror(errno)));
  }
}

void RawTable::impl::remove(const void *key) {
  if (bpf_delete_elem(fd_, const_cast<void *>(key))) {
    throw std::runtime_error("Table remove error: " +
      std::string(std::strerror(errno)));
  }
}

int RawTable::impl::first(void *key) {
  /* TODO: where to get key size from? */
  return bpf_get_first_key(fd_, key, 32/*sizeof(first)*/);
}

int RawTable::impl::next(const void *key, void *next) {
  return bpf_get_next_key(fd_, const_cast<void *>(key), next);
}

// PIMPL
RawTable::~RawTable() {}

RawTable::RawTable(void *op): pimpl_(new impl(op)) {}

void RawTable::get(const void *key, void *value) {
  return pimpl_->get(key, value);
}

void RawTable::set(const void *key, const void *value) {
  return pimpl_->set(key, value);
}

void RawTable::remove(const void *key) {
  return pimpl_->remove(key);
}

int RawTable::first(void *key) {
  return pimpl_->first(key);
}

int RawTable::next(const void *key, void *next) {
  return pimpl_->next(key, next);
}

}  // namespace service
}  // namespace polycube
