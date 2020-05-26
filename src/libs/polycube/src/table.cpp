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

#include <libbpf/src/bpf.h>
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

  int pop(void *value);
  int push(const void *value);

 private:
  bool is_queue_stack_enabled;
  bool is_batch_enabled;
  int fd_;
};

RawTable::impl::~impl() {
}

RawTable::impl::impl(void *op) : fd_(*(int *)op) {
  auto version = get_kernel_release();
  is_queue_stack_enabled = compare_kernel_release(version, QUEUE_STACK_KERNEL_RELEASE);
  is_batch_enabled = compare_kernel_release(version, BATCH_KERNEL_RELEASE);
}

void RawTable::impl::get(const void *key, void *value) {
  if (bpf_lookup_elem(fd_, const_cast<void *>(key),
                      const_cast<void *>(value))) {
    throw std::runtime_error("Table get error: " +
                             std::string(std::strerror(errno)));
  }
}

void RawTable::impl::set(const void *key, const void *value) {
  if (bpf_update_elem(fd_, const_cast<void *>(key), const_cast<void *>(value),
                      0)) {
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
  return bpf_get_first_key(fd_, key, 32 /*sizeof(first)*/);
}

int RawTable::impl::next(const void *key, void *next) {
  return bpf_get_next_key(fd_, const_cast<void *>(key), next);
}

int RawTable::impl::pop(void *value) {
  if(!is_queue_stack_enabled)
    throw std::runtime_error("BPF map POP operation is not supported."
                           "Update your kernel to version 5.0.0");
  return bpf_map_lookup_and_delete_elem(fd_, nullptr, value);
}

int RawTable::impl::push(const void *value) {
  if(!is_queue_stack_enabled)
    throw std::runtime_error("BPF map PUSH operation is not supported."
                           "Update your kernel to version 5.0.0");
  return bpf_map_update_elem(fd_, nullptr, value, 0);
}

// PIMPL
RawTable::~RawTable() {}

RawTable::RawTable(void *op) : pimpl_(new impl(op)) {}

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

int RawTable::pop(void *value) {
  return pimpl_->pop(value);
}

int RawTable::push(const void *value) {
  return pimpl_->push(value);
}

}  // namespace service
}  // namespace polycube
