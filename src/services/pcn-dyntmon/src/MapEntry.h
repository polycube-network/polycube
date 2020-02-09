#pragma once

#include <memory>
#include <vector>

class MapEntry {
 public:
  MapEntry(std::size_t key_size, std::size_t value_size) {
    key = calloc(1, key_size);
    value = calloc(1, value_size);
  }

  MapEntry(MapEntry &entry) {
    this->key = entry.key;
    this->value = entry.value;
    entry.key = NULL;
    entry.value = NULL;
  }

  ~MapEntry() {
    if (!key)
      free(key);
    if (!value)
      free(value);
  }

  void *getKey() {
    return key;
  }

  void *getValue() {
    return value;
  }

 private:
  void *key = NULL;
  void *value = NULL;
};
