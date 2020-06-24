#include "SwapStateConfig.h"

#include <utility>

#include "CodeRewriter.h"

SwapStateConfig::SwapStateConfig()  : compileType(CompileType::NONE), current_program_index(1) {}

SwapStateConfig::SwapStateConfig(std::string original_code, std::string swapped_code,
                                 CompileType compileType, std::string master_code,
                                 std::unordered_map<std::string,std::string> names,
                                 std::unordered_map<std::string, std::string> global_names)
    : current_program_index(1), compileType(compileType),
      codes({std::move(master_code), std::move(original_code), std::move(swapped_code)}),
      enhanced_names(std::move(names)), enhanced_global_names(std::move(global_names)){}

int SwapStateConfig::getProgramIndexAndMapNameToRead(std::string &mapName) {
  int index = 0;
  switch (compileType) {
  case CompileType::NONE:
    break;
  case CompileType::ENHANCED: {
    /*Check if map has tuned name or, in case the "original" program is loaded, it has
     * a globally tuned name among INGRESS-EGRESS */
    auto tuned = enhanced_names.find(mapName);
    if(tuned != enhanced_names.end() ||
        (current_program_index == 1 && (tuned= enhanced_global_names.find(mapName)) != enhanced_global_names.end()))
      mapName = tuned->second;
    index = (current_program_index % 2) + 1;
    break;
  }
  case CompileType::BASE: {
    /* Check if the map has a tuned name */
    auto tuned = enhanced_names.find(mapName);
    if(current_program_index == 1 && tuned != enhanced_names.end())
      mapName = tuned->second;
  }
  }
  return index;
}
