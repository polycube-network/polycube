#include "MapExtractor.h"

#include <functional>
#include <string>

#include <linux/bpf.h>

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

#pragma region compiletime_string_hashing
/**
 * This region of code exports the hash method which is used at compile time to
 * transform a string into uint64_t value by hashing it; this permits to use
 * the switch statement on strings.
 */
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
#pragma endregion

using namespace StringHash;

#define ENUM hash("enum")
#define UNION hash("union")
#define STRUCT hash("struct")
#define STRUCT_PACKED hash("struct_packed")
#define PADDING "__pad_"

typedef enum {
  Property,
  ArrayProperty,
  Value,
  Struct,
  Union,
  Enum,
} objectType;

/**
 * Recognizes the type of a node object contained in a TableDesc's leaf_desc tree
 * 
 * @param[object] the object to be recognized
 * 
 * @throw std::runtime_error if the object cannot be recognized
 * 
 * @returns a objectType
 */
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
        case STRUCT_PACKED:
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
      else if (object[2].is_number() && object[1].is_string())
        return Property;
      throw runtime_error("Unknown element" + object[0].get<string>());
    }
  }
  throw runtime_error("Unable to identifyObject " + object.dump());
}

json MapExtractor::extractFromLinearMap(const TableDesc &desc, const RawTable& table) {

  // Getting the leaf_desc object which represents the structure of the a map entry
  auto value_desc = json::parse(string{desc.leaf_desc});

  // Getting the map entries (for these maps only the value is considered)
  auto entries = getMapEntries(table, desc.key_size, desc.leaf_size, desc.type != BPF_MAP_TYPE_ARRAY);

  json j_entries;
  for(auto &entry: entries) {
    int offset = 0;
    auto j_entry = recExtract(value_desc, entry->getValue(), offset);
    j_entries.push_back(j_entry);
  }
  return j_entries;
}

json MapExtractor::extractFromHashMap(const TableDesc &desc, const RawTable& table) {

  // Getting both key_desc and leaf_desc objects which represents the structure of the map entry
  auto value_desc = json::parse(string{desc.leaf_desc});
  auto key_desc = json::parse(string{desc.key_desc});

  // Getting the map entries (both key and value are considered)
  auto entries = getMapEntries(table, desc.key_size, desc.leaf_size);

  json j_entries;
  for (auto &entry : entries) {
    int val_offset = 0, key_offset = 0;
    json entry_obj;
    entry_obj["key"] = recExtract(key_desc, entry->getKey(), key_offset);
    entry_obj["value"] = recExtract(value_desc, entry->getValue(), val_offset);
    j_entries.push_back(entry_obj);
  }
  return j_entries;
}

json MapExtractor::extractFromPerCPUMap(const TableDesc &desc, const RawTable &table) {
  // Getting the maximux cpu available
  size_t n_cpus = polycube::get_possible_cpu_count();

  // Getting the map entries
  auto entries = getMapEntries(table, desc.key_size, desc.leaf_size * n_cpus);

  // Getting key and value description to parse correctly
  auto value_desc = json::parse(string{desc.leaf_desc});
  auto key_desc = json::parse(string{desc.key_desc});

  json j_entries;
  for(auto& entry : entries) {
    int key_offset = 0;
    json j_entry;

    // Parse the current key and set the id of that entry
    j_entry["key"] = recExtract(key_desc, entry->getKey(), key_offset);

    json j_values;
    // The leaf_size is the size of a single leaf value, but potentially there is an
    // array of values! So the entry->getValue() pointer must be manually shifted to parse
    // all the per_cpu values
    for(int i=0, offset=0; i<n_cpus; i++, offset=0){
      auto val = recExtract(value_desc, static_cast<char*>(entry->getValue()) + (i * desc.leaf_size), offset);
      j_values.emplace_back(val);
    }
    j_entry["value"] = j_values;
    j_entries.push_back(j_entry);
  }
  return j_entries;
}

json MapExtractor::extractFromMap(BaseCube &cube_ref, const string& map_name, int index,
                                  ProgramType type) {
  // Getting the TableDesc object of the eBPF table
  auto &desc = cube_ref.get_table_desc(map_name, index, type);

  if(desc.type == BPF_MAP_TYPE_HASH || desc.type == BPF_MAP_TYPE_LRU_HASH)
    return extractFromHashMap(desc, cube_ref.get_raw_table(map_name, index, type));
  else if(desc.type == BPF_MAP_TYPE_ARRAY || desc.type == BPF_MAP_TYPE_QUEUE || desc.type == BPF_MAP_TYPE_STACK)
    return extractFromLinearMap(desc, cube_ref.get_raw_table(map_name, index, type));
  else if(desc.type == BPF_MAP_TYPE_PERCPU_HASH || desc.type == BPF_MAP_TYPE_PERCPU_ARRAY || desc.type == BPF_MAP_TYPE_LRU_PERCPU_HASH)
    return extractFromPerCPUMap(desc, cube_ref.get_raw_table(map_name, index, type));
  else
    // The map type is not supported yet by Dynmon
    throw runtime_error("Unhandled Map Type " + std::to_string(desc.type) + " extraction.");
}

std::vector<std::shared_ptr<MapEntry>> MapExtractor::getMapEntries(
    RawTable table, size_t key_size, size_t value_size, bool isQueueStack) {
  std::vector<std::shared_ptr<MapEntry>> vec;

  // If it is a queue/stack then POP should be used, otherwise normal lookup
  if(isQueueStack) {
    // Looping until pop extracts something
    while(true) {
      MapEntry entry(0, value_size);
      if (table.pop(entry.getValue()) != 0)
        break;
      vec.push_back(std::make_shared<MapEntry>(entry));
    }
  } else {
    void *last_key = nullptr;

    // Looping until the RawTable has a "next" entry
    while (true) {
      // Creating a MapEntry object which internally allocates the memory to
      // contain the map entry's key and value
      MapEntry entry(key_size, value_size);
      int fd = table.next(last_key, entry.getKey());
      last_key = entry.getKey();
      if (fd > -1) {
        table.get(entry.getKey(), entry.getValue());
        vec.push_back(std::make_shared<MapEntry>(entry));
      } else
        break;
    }
  }
  return vec;
}

json MapExtractor::recExtract(json object_description, void *data, int &offset) {
  // Identifying the object
  auto objectType = identifyObject(object_description);

  switch (objectType) {
  case Property: { // the object describes a property of a C struct
    auto propertyName = object_description[0].get<string>();
    auto propertyType = object_description[1];
    return propertyName, recExtract(propertyType, data, offset);
  }
  case ArrayProperty: { // the object describes a property of a C struct which is an array
    auto propertyName = object_description[0].get<string>();
    auto propertyType = object_description[1];
    auto len = object_description[2][0].get<int>();
    // array of primitive type 
    if (propertyType.is_string()) // this string is the name of the primitive type
      return valueFromPrimitiveType(propertyType, data, offset, len);
    // array of complex type
    else {
      json array;
      for (int i = 0; i < len; i++)
        array.push_back(recExtract(propertyType, data, offset));
      return value_type(propertyName, array);
    }
  }
  case Value: // the object describes a primitive type
    return valueFromPrimitiveType(object_description.get<string>(), data,
                                  offset);
  case Struct: // the object describes a C struct
    return valueFromStruct(object_description, data, offset);
  
  case Union: // the object describes a C union
    return valueFromUnion(object_description, data, offset);
  
  case Enum: // the object describes a C enum
    return valueFromEnum(object_description, data, offset);
  }
}

json MapExtractor::valueFromStruct(json struct_description, void *data,
                                   int &offset) {
  json array;
  // Getting the name of the C struct
  // auto structName = struct_description[0].get<string>();
  // Getting the set of properties of the C struct
  auto structProperties = struct_description[1];

  // For each property
  for (auto &property : structProperties) {
    // Getting the property's name
    auto propertyName = property[0].get<std::string>();
    // Getting the property type object
    // auto propertyType = property[1];
    // If the property is an alignment padding -> skipp it increasing the offset
    if (propertyName.rfind(PADDING, 0) == 0) {
      auto paddingSize = property[2][0].get<int>();
      offset += paddingSize;
    } else
      // Adding the property to the returning set extracting its value recursively
      array.push_back(value_type(propertyName, recExtract(property, data, offset)));
  }
  return array;
}

json MapExtractor::valueFromUnion(json union_description, void *data,
                                  int &offset) {
  json array;
  int oldOffset = offset;
  int maxOffset = 0;
  // Getting the name of the C union
  // auto unionName = union_description[0].get<string>();
  // Getting the set of properties of the C union
  auto unionProperties = union_description[1];

  // For each property
  for (auto &property : unionProperties) {
    // Getting the property's name
    auto propertyName = property[0].get<std::string>();
    // Getting the property type object
    // auto propertyType = property[1];
    // Adding the property to the returning set extracting its value recursively
    array.push_back(value_type(propertyName, recExtract(property, data, offset)));
    // Saving the offset in order to increase it to the maximum property size
    if (offset > maxOffset)
      maxOffset = offset;
    // Restoring the previous offset before iterate to the next property
    offset = oldOffset;
  }
  // Setting the offset to the maximum found so far
  offset = maxOffset;
  return array;
}

json MapExtractor::valueFromEnum(json enum_description, void *data,
                                 int &offset) {
  int index = *(int *)((char *)data + offset);
  offset += sizeof(int);
  return enum_description[1][index].get<string>();
}

json MapExtractor::valueFromPrimitiveType(const string& type_name, void *data,
                                          int &offset, int len) {
  char *address = ((char *)data + offset);
  // Casting the memory pointed by *address to the corresponding C type
  switch (hash(type_name)) {
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
      json vec = json::array();
      for (int i = 0; i < len; i++) {
        char value = *address;
        offset += sizeof(char);
        address += sizeof(char);
        vec.push_back(value);
      }
      return vec;
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
    throw runtime_error("ERROR: " + type_name + " not a valid type!");
  }
}
