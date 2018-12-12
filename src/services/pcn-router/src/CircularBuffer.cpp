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

#include "CircularBuffer.h"
CircularBuffer::CircularBuffer() {
  front = -1;
  rear = -1;
  last_access =
      duration_cast<milliseconds>(system_clock::now().time_since_epoch());
}

bool CircularBuffer::isFull() {
  if (front == 0 && rear == SIZE - 1) {
    return true;
  }
  if (front == rear + 1) {
    return true;
  }
  return false;
}

bool CircularBuffer::isEmpty() {
  if (front == -1)
    return true;
  else
    return false;
}

void CircularBuffer::enQueue(std::vector<uint8_t> element) {
  if (isFull()) {
    front++;
  }

  if (front == -1)
    front = 0;

  rear = (rear + 1) % SIZE;
  items[rear] = element;
  last_access =
      duration_cast<milliseconds>(system_clock::now().time_since_epoch());
}

std::vector<uint8_t> CircularBuffer::deQueue() {
  std::vector<uint8_t> element;
  if (isEmpty()) {
    return element;
  } else {
    element = items[front];
    if (front == rear) {
      front = -1;
      rear = -1;
    } /* Q has only one element, so we reset the queue after deleting it. */
    else {
      front = (front + 1) % SIZE;
    }
    return (element);
  }
}

std::vector<std::vector<uint8_t>> CircularBuffer::get_elements() {
  int i;
  std::vector<std::vector<uint8_t>> vect;
  if (isEmpty()) {
    return vect;
  } else {
    for (i = front; i != rear; i = (i + 1) % SIZE)
      vect.push_back(items[i]);

    vect.push_back(items[i]);
  }
  return vect;
}
