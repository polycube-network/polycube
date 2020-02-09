#include "MapExtractor.h"

#include <functional>
#include <iostream>
#include <string>

#include "polycube/services/table_desc.h"

#define INT "int"
#define CHAR "char"
#define SHORT "short"
#define LONG "long"
#define FLOAT "float"
#define DOUBLE "double"
#define LONG_LONG "long long"
#define LONG_DOUBLE "long double"
#define SIGNED_CHAR "signed char"
#define UNSIGNED_INT "unsigned int"
#define UNSIGNED_CHAR "unsigned char"
#define UNSIGNED_LONG "unsigned long"
#define UNSIGNED_SHORT "unsigned short"
#define UNSIGNED_LONG_LONG "unsigned long long"

using std::ostringstream;
using std::runtime_error;
using namespace polycube::service;
using json = nlohmann::json;
using value_type = json::object_t::value_type;

namespace StringHash {
constexpr uint64_t _(uint64_t h, const char *s) {
  return (*s == 0)
             ? h
             : _((h * 1099511628211ull) ^ static_cast<uint64_t>(*s), s + 1);
}

constexpr uint64_t hash(const char *s) {
  return StringHash::_(14695981039346656037ull, s);
}

uint64_t hash(const std::string &s) {
  return StringHash::_(14695981039346656037ull, s.data());
}
}  // namespace StringHash

using namespace StringHash;

#define ENUM hash("enum")
#define UNION hash("union")
#define STRUCT hash("struct_packed")
#define PADDING "__pad_"

typedef enum {
  Property,
  ArrayProperty,
  Value,
  Struct,
  Union,
  Enum,
} objectType;

objectType identifyObject(json object) {
  if (object.is_string())
    return Value;
  if (object.is_array()) {
    if (object.size() == 2)
      return Property;
    if (object.size() == 3) {
      if (object[2].is_string()) {
        switch (hash(object[2])) {
        case STRUCT:
          return Struct;
        case UNION:
          return Union;
        case ENUM:
          return Enum;
        default:
          throw runtime_error("Unknown type" + object[3].get<string>());
        }
      } else if (object[2].is_array())
        return ArrayProperty;
      throw runtime_error("Unknown element" + object[0].get<string>());
    }
  }
}

json MapExtractor::extractFromMap(BaseCube &cubeRef, string mapName, int index,
                                  ProgramType type) {
  auto &desc = cubeRef.get_table_desc(mapName, index, type);

  auto value_desc = json::parse(string{desc.leaf_desc});
  // std::cout << std::endl << value_desc.dump(2) << std::endl;
  auto table = cubeRef.get_raw_table(mapName, index, type);
  auto entries = getMapEntries(table, desc.key_size, desc.leaf_size);
  json j_entries;
  for (auto &entry : entries) {
    int offset = 0;
    auto j_entry = recExtract(value_desc, entry->getValue(), offset);
    j_entries.push_back(j_entry);
  }
  return j_entries;
}

std::vector<std::shared_ptr<MapEntry>> MapExtractor::getMapEntries(
    RawTable table, size_t key_size, size_t value_size) {
  std::vector<std::shared_ptr<MapEntry>> vec;
  void *last_key = nullptr;
  while (true) {
    MapEntry entry(key_size, value_size);
    int fd = table.next(last_key, entry.getKey());
    last_key = entry.getKey();
    if (fd > -1) {
      table.get(entry.getKey(), entry.getValue());
      vec.push_back(std::make_shared<MapEntry>(entry));
    } else
      break;
  }
  return vec;
}

// Identifies an object type and call the corresponding extraction method
// Returns a json which represents an object filled with the data
json MapExtractor::recExtract(json objectDescription, void *data, int &offset) {
  auto objectType = identifyObject(objectDescription);

  switch (objectType) {
  case Property: {
    auto propertyName = objectDescription[0].get<string>();
    auto propertyType = objectDescription[1];
    return propertyName, recExtract(propertyType, data, offset);
  }
  case ArrayProperty: {
    auto propertyName = objectDescription[0].get<string>();
    auto propertyType = objectDescription[1];
    auto len = objectDescription[2][0].get<int>();
    // array of primitive type
    if (propertyType.is_string())
      return valueFromPrimitiveType(propertyType, data, offset, len);
    // array of complex type
    else {
      json array;
      for (int i = 0; i < len; i++)
        array.push_back(recExtract(propertyType, data, offset));
      return value_type(propertyName, array);
    }
  }
  case Value:
    return valueFromPrimitiveType(objectDescription.get<string>(), data,
                                  offset);
  case Struct: {
    return valueFromStruct(objectDescription, data, offset);
  }
  case Union: {
    return valueFromUnion(objectDescription, data, offset);
  }
  case Enum: {
    return valueFromEnum(objectDescription, data, offset);
  }
  }
}

// extract the value of a struct as an array of properties
json MapExtractor::valueFromStruct(json structDescription, void *data,
                                   int &offset) {
  json array;
  auto structName = structDescription[0].get<string>();
  auto structProperties = structDescription[1];
  for (auto &property : structProperties) {
    auto propertyName = property[0].get<std::string>();
    auto propertyType = property[1];
    if (propertyName.rfind(PADDING, 0) == 0) {
      // the property is an alignment padding -> skip
      auto paddingSize = property[2][0].get<int>();
      offset += paddingSize;
    } else
      array.push_back(
          value_type(propertyName, recExtract(property, data, offset)));
  }
  return array;
}

json MapExtractor::valueFromUnion(json unionDescription, void *data,
                                  int &offset) {
  json array;
  int oldOffset = offset;
  int maxOffset = 0;

  auto unionName = unionDescription[0].get<string>();
  auto unionProperties = unionDescription[1];
  for (auto &property : unionProperties) {
    auto propertyName = property[0].get<std::string>();
    auto propertyType = property[1];
    array.push_back(
        value_type(propertyName, recExtract(property, data, offset)));
    if (offset > maxOffset)
      maxOffset = offset;
    offset = oldOffset;
  }
  offset = maxOffset;
  return array;
}

json MapExtractor::valueFromEnum(json enumDescription, void *data,
                                 int &offset) {
  int index = *(int *)((char *)data + offset);
  offset += sizeof(int);
  return enumDescription[1][index].get<string>();
}

json MapExtractor::valueFromPrimitiveType(const string typeName, void *data,
                                          int &offset, int len) {
  char *address = ((char *)data + offset);
  switch (hash(typeName)) {
  case hash(INT): {
    if (len == -1) {
      int value = *(int *)address;
      offset += sizeof(int);
      return value;
    } else {
      json vec = json::array();
      for (int i = 0; i < len; i++) {
        int value = *(int *)address;
        offset += sizeof(int);
        address += sizeof(int);
        vec.push_back(value);
      }
      return vec;
    }
  }
  case hash(CHAR): {
    if (len == -1) {
      char value = *address;
      offset += sizeof(char);
      return value;
    } else {
      char *value = address;
      offset += sizeof(char) * len;
      return value;
    }
  }
  case hash(SHORT): {
    if (len == -1) {
      short value = *(short *)address;
      offset += sizeof(short);
      return value;
    } else {
      json vec = json::array();
      for (int i = 0; i < len; i++) {
        short value = *(short *)address;
        offset += sizeof(short);
        address += sizeof(short);
        vec.push_back(value);
      }
      return vec;
    }
  }
  case hash(LONG): {
    if (len == -1) {
      long value = *(long *)address;
      offset += sizeof(long);
      return value;
    } else {
      json vec = json::array();
      for (int i = 0; i < len; i++) {
        long value = *(long *)address;
        offset += sizeof(long);
        address += sizeof(long);
        vec.push_back(value);
      }
      return vec;
    }
  }
  case hash(FLOAT): {
    if (len == -1) {
      float value = *(float *)address;
      offset += sizeof(float);
      return value;
    } else {
      json vec = json::array();
      for (int i = 0; i < len; i++) {
        float value = *(float *)address;
        offset += sizeof(float);
        address += sizeof(float);
        vec.push_back(value);
      }
      return vec;
    }
  }
  case hash(DOUBLE): {
    if (len == -1) {
      double value = *(double *)address;
      offset += sizeof(double);
      return value;
    } else {
      json vec = json::array();
      for (int i = 0; i < len; i++) {
        double value = *(double *)address;
        offset += sizeof(double);
        address += sizeof(double);
        vec.push_back(value);
      }
      return vec;
    }
  }
  case hash(LONG_LONG): {
    if (len == -1) {
      long long value = *(long long *)address;
      offset += sizeof(long long);
      return value;
    } else {
      json vec = json::array();
      for (int i = 0; i < len; i++) {
        long long value = *(long long *)address;
        offset += sizeof(long long);
        address += sizeof(long long);
        vec.push_back(value);
      }
      return vec;
    }
  }
  case hash(LONG_DOUBLE): {
    if (len == -1) {
      long double value = *(long double *)address;
      offset += sizeof(long double);
      return value;
    } else {
      json vec = json::array();
      for (int i = 0; i < len; i++) {
        long double value = *(long double *)address;
        offset += sizeof(long double);
        address += sizeof(long double);
        vec.push_back(value);
      }
      return vec;
    }
  }
  case hash(SIGNED_CHAR): {
    if (len == -1) {
      signed char value = *address;
      offset += sizeof(signed char);
      return value;
    } else {
      signed char *value = (signed char *)address;
      offset += sizeof(signed char) * len;
      return (char *)value;
    }
  }
  case hash(UNSIGNED_INT): {
    if (len == -1) {
      unsigned int value = *(unsigned int *)address;
      offset += sizeof(unsigned int);
      return value;
    } else {
      json vec = json::array();
      for (int i = 0; i < len; i++) {
        unsigned int value = *(unsigned int *)address;
        offset += sizeof(unsigned int);
        address += sizeof(unsigned int);
        vec.push_back(value);
      }
      return vec;
    }
  }
  case hash(UNSIGNED_CHAR): {
    if (len == -1) {
      unsigned char value = *address;
      offset += sizeof(unsigned char);
      return value;
    } else {
      unsigned char *value = (unsigned char *)address;
      offset += sizeof(unsigned char) * len;
      return (char *)value;
    }
  }
  case hash(UNSIGNED_LONG): {
    if (len == -1) {
      unsigned long value = *(unsigned long *)address;
      offset += sizeof(unsigned long);
      return value;
    } else {
      json vec = json::array();
      for (int i = 0; i < len; i++) {
        unsigned long value = *(unsigned long *)address;
        offset += sizeof(unsigned long);
        address += sizeof(unsigned long);
        vec.push_back(value);
      }
      return vec;
    }
  }
  case hash(UNSIGNED_SHORT): {
    if (len == -1) {
      unsigned short value = *(unsigned short *)address;
      offset += sizeof(unsigned short);
      return value;
    } else {
      json vec = json::array();
      for (int i = 0; i < len; i++) {
        unsigned short value = *(unsigned short *)address;
        offset += sizeof(unsigned short);
        address += sizeof(unsigned short);
        vec.push_back(value);
      }
      return vec;
    }
  }
  case hash(UNSIGNED_LONG_LONG): {
    if (len == -1) {
      unsigned long long value = *(unsigned long long *)address;
      offset += sizeof(unsigned long long);
      return value;
    } else {
      json vec = json::array();
      for (int i = 0; i < len; i++) {
        unsigned long long value = *(unsigned long long *)address;
        offset += sizeof(unsigned long long);
        address += sizeof(unsigned long long);
        vec.push_back(value);
      }
      return vec;
    }
  }
  default:
    throw runtime_error("ERROR: " + typeName + " not a valid type!");
  }
}
