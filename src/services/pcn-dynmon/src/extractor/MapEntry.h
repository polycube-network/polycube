#pragma once

#include <memory>
#include <vector>

/**
 * This class represents a generic entry in a eBPF map;
 * it stores two native pointers to memory spaces allocated to store the entry's key and value.
 */
class MapEntry {
 public:
  /**
   * The class constructor allocates the memory to hold the entry's key and value.
   *
   * @param[key_size] represents the size of the entry's key
   * @param[value_size] represents the size of the entry's value
   */
  MapEntry(std::size_t key_size, std::size_t value_size) {
    key = calloc(1, key_size);
    value = calloc(1, value_size);
  }

  /**
   * When a MapEntry object is copied, only the copy stores the actual pointers.
   */
  MapEntry(MapEntry &source) {
    this->key = source.key;
    this->value = source.value;
    source.key = NULL;
    source.value = NULL;
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
