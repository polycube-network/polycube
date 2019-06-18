/*
 * Copyright 2019 The Polycube Authors
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

#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>

namespace polycube::polycubed {

class Namespace {
 public:
  static Namespace create(const std::string &name);
  static Namespace open(const std::string &name);

  void execute(std::function<void()> f);
  void remove();
  void set_id(int id);
  int set_random_id();

  std::string get_name() const;
  int get_fd() const;

  ~Namespace();

 private:
  Namespace(const std::string &name, int fd);
  static void create_ns(const std::string &name);

  static void execute_in_worker(std::function<void()> f);

  static void stop();
  static void start();

  std::string name_;
  int fd_;

  struct Event;

  static std::unique_ptr<std::thread> executor;
  static std::condition_variable cv;
  static std::mutex mutex;
  static std::queue<Event*> queue;
  static bool finished;

  static std::atomic<int> count;

  static void worker();
};

} // namespace polycube::polycube
