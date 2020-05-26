#pragma once

#include "MapEntry.h"
#include "polycube/services/base_cube.h"  // polycube::service::BaseCube
#include "polycube/services/json.hpp"     // nlohmann::json
#include "polycube/services/table.h"      // polycube::service::RawTable

using ebpf::TableDesc;
using namespace polycube::service;
using polycube::service::RawTable;
using spdlog::logger;
using std::shared_ptr;
using std::string;
using json = nlohmann::json;

/**
 * MapExtractor is a component able to extract the content of a eBPF map in a
 * JSON format.
 *
 * This component exploit the use of the TableDesc object which
 * describes how a eBPF map is built. The TableDesc object of a map is obtained
 * by calling the 'get_table_desc' method exposed by a Polycube BaseCube.
 *
 * A TableDesc object contains a leaf_desc object, a JSON which describes the
 * structure of the entries of the eBPF map.
 *
 * This JSON object has a predefined structure to represent C structs, unions,
 * arrays, enums and primitive types (such as "int", "char", "double", "unsigned
 * long long" and so on). The structure of this object is different for each one
 * of the aforementioned C data types.
 *
 * In order to extract the content of a eBPF map, the leaf_desc object is parsed
 * recursively to cast the memory block of the map's entries accordingly to
 * their structure.
 *
 * The parsing recursion of the leaf_desc JSON object first recognizes it as one
 * of the aforementioned C data types, then calls the corresponding
 * extraction method which will cast the memory block of a map entry to
 * extract the contained value.
 * Since in the leaf_desc object structs and unions have nested data types for
 * each one of their fields, the recursive method is called on each of them.
 */
class MapExtractor {
 public:
  /**
   * Wrapper method for the extration of the content of a eBPF map.
   *
   * This method obtains the TableDesc object corresponding to a eBPF map and
   * calls, for each map entry, the recursive extraction method 'recExtract'.
   *
   * The obtained objects are grouped in a single JSON which is returned as
   * output.
   *
   * @param[cube_ref] reference to a Polycube cube
   * @param[map_name] name of the eBPF map
   * @param[index] index of the eBPF program which declares the map
   * @param[type] type of the eBPF program (INGRESS or EGRESS)
   *
   * @returns the content of a map as a JSON object
   */
  static json extractFromMap(BaseCube &cube_ref, string map_name, int index = 0,
                             ProgramType type = ProgramType::INGRESS);

 private:
  MapExtractor() = default;

  ~MapExtractor() = default;

  /**
   * Returns a vector of MapEntry objects which internally contain the pointers
   * to the map entries keys and values.
   *
   * @param[table] a RawTable object corresponding to the eBPF table
   * @param[key_size] the size of the table keys
   * @param[value_size] the size of the table values
   * @param[isQueueStack] indicates whether the map has to be treated like a Queue/Stack
   *
   * @returns a vector of MapEntry objects
   */
  static std::vector<std::shared_ptr<MapEntry>> getMapEntries(
      RawTable table, size_t key_size, size_t value_size, bool isQueueStack = false);

  /**
   * Recursive method which identifies the type of an object
   * (obtained from a TableDesc's leaf_desc object) and calls
   * the corrisponding extraction method.
   *
   * @param[type_description] description of a node type in the TableDesc object
   * @param[data] pointer to a memory block
   * @param[offset] offset from the begin of the memory pointed by @data
   *
   * @returns a JSON object which represents the content of the map entry
   */
  static json recExtract(json type_description, void *data, int &offset);

  /**
   * Parses a memory block corresponding to a C struct and produces a JSON
   * object which represents the struct as a set of key-value pairs where the
   * keys are the names of the struct properties and the values are the results
   * of the memory casting operation.
   *
   * For a struct like this:
   * struct mystruct{
   *   uint64_t property1;
   *   unsigned long property2;
   * }
   *
   * the leaf_desc JSON object would contain:
   * [
   *  "mystruct",
   *  [
   *   ["property1", "unsigned long long"],
   *   ["property2", "unsigned long" ]
   *  ],
   *  "struct_packed"
   * ]
   *
   * where the second element of the outer vector corresponds to the
   * @param strut_properties.
   *
   * The output JSON object would contain:
   *
   * "{
   *   "property1": *value*,
   *   "property2": *value*
   * }"
   *
   * @param[struct_properties] the proerties of the struct
   * @param[data] pointer to a memory block
   * @param[offset] offset from the begin of the memory pointed by @data
   *
   * @returns a JSON object which represents the value of the C struct
   */
  static json valueFromStruct(json struct_properties, void *data, int &offset);

  /**
   * Parses a memory block corresponding to a C union.
   * The real type of the union is decided by the program wich is using the
   * union and cannot be known at runtime so the memory corresponding to the
   * union value is here parsed as each possible type the union contains in
   * order to let decide who uses the union how to handle its value.
   *
   * This method produces a JSON object which represents the union value as a
   * set of key-value pairs where the keys are the names of the possible types
   * and the values are the results of the memory casting operation.
   *
   * For a union like this:
   * union myunion{
   *     uint64_t type1;
   *     char type2:
   * }
   *
   * the leaf_desc JSON object would contain:
   * [
   *  "myunion",
   *  [
   *   ["type1", "unsigned long long"],
       ["type2", "char"]
  *   ],
   *  "union"
   * ]
   *
   * where the second element of the outer vector corresponds to the
   * @param union_types.
   *
   * The output JSON object would contain:
   * "{
   *   "type1": *value*, <- casted as unsigned long long
   *   "type2": *value*  <- casted as char
   * }"
   *
   * @param[union_types] the possible types of the union
   * @param[data] pointer to a memory block
   * @param[offset] offset from the begin of the memory pointed by @data
   *
   * @returns a JSON object which represents the value of the C union
   */
  static json valueFromUnion(json union_types, void *data, int &offset);

  /**
   * Parses a memory block corresponding to a C enum and produces a JSON object
   * which represents the enum.
   *
   * For a enum like this:
   * enum myenum{
   *     TCP, UDP, HTTP
   * }
   *
   * the leaf_desc JSON object would contain:
   * [
   *  "myenum",
   *  ["TCP", "UDP", "HTTP"],
   *  "enum"
   * ]
   *
   * which corresponds to the @param enum_values.
   *
   * The casting operation will look at the value stored in the memory block at
   * address @param data+ @param offset and will use it as an index to access to
   * the second element of the JSON object to return the corrisponding enum
   * value (e.g., "TCP")
   *
   * @param[union_types] the possible value of the enum
   * @param[data] pointer to a memory block
   * @param[offset] offset from the begin of the memory pointed by @data
   *
   * @returns a JSON object which represents the value of the C enum
   */
  static json valueFromEnum(json enum_values, void *data, int &offset);

  /**
   * Parses a memory block corresponding to a C primitive type (e.g., int, uint,
   * float, double, char, ecc...) and produces a JSON object which represents
   * the value resulting from the memory casting operation.
   *
   * For each primitive type supported by eBPF, a corresponding string name is
   * contained in the leaf_desc JSON object (e.g., "unsigned long long", "signed
   * char", etc..).
   *
   * The casting operation will use this string to cast the memory block
   * correctly and return the contained value.
   *
   * @param[union_types] the name of the primitive type
   * @param[data] pointer to a memory block
   * @param[offset] offset from the begin of the memory pointed by @data
   * @param[len] indicates the number of elements the value if it is an array;
   * it is -1 if the value is not an array
   *
   * @returns a JSON object which represents the value
   */
  static json valueFromPrimitiveType(string type_name, void *data, int &offset,
                                     int len = -1);
};
