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

class MapExtractor {
 public:
  static json extractFromMap(BaseCube &cubeRef, string mapName, int index = 0,
                             ProgramType type = ProgramType::INGRESS);

 private:
  MapExtractor() = default;

  ~MapExtractor() = default;

  static std::vector<std::shared_ptr<MapEntry>> getMapEntries(
      RawTable table, size_t key_size, size_t value_size);

  static json recExtract(json typeDescription, void *data, int &offset);

  static json valueFromStruct(json structProperties, void *data, int &offset);

  static json valueFromUnion(json unionTypes, void *data, int &offset);

  static json valueFromEnum(json enumValues, void *data, int &offset);

  static json valueFromPrimitiveType(string typeName, void *data, int &offset, int len = -1);
};
