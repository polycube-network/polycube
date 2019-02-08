
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

#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>
#define SIZE 5 /* Size of Circular Queue */

using namespace std;
using namespace std::chrono;

class CircularBuffer {
 private:
  std::vector<uint8_t> items[SIZE];
  int front, rear;
  milliseconds last_access;

 public:
  CircularBuffer();
  bool isFull();
  bool isEmpty();
  void enQueue(std::vector<uint8_t> element);
  std::vector<uint8_t> deQueue();
  std::vector<std::vector<uint8_t>> get_elements();
  milliseconds get_last_access() {
    return last_access;
  };
};
