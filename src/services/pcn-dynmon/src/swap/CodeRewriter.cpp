#include "CodeRewriter.h"

#include <polycube/services/cube_iface.h>

#include <regex>
#include <unordered_map>

/*The suffix to be added to the original NON_SWAPPABLE map name when swapped
 * in the EGRESS program and CompileType=ENHANCED*/
const char ENHANCED_SWAP_MAP_NAME_FORMAT_EGRESS[]  = "_egress";

/*The suffix to be added to the original NON_SWAPPABLE map name when swapped
 * in the INGRESS program and CompileType=ENHANCED*/
const char ENHANCED_SWAP_MAP_NAME_FORMAT_INGRESS[] = "_ingress";

/*The suffix to be added to the original SWAPPABLE map name when swapped and CompileType=BASE*/
const char BASE_SWAP_MAP_NAME_FORMAT[]             = "_1";

/*Dynmon master code to be injected when swap is enabled*/
const char dynmon_swap_master[] = R"(
      BPF_ARRAY(INTERNAL_PROGRAM_INDEX, int, 1);
      static __always_inline int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
       unsigned key = 0;
       int *index = INTERNAL_PROGRAM_INDEX.lookup(&key);
       if(!index) {
         return RX_OK;
        }
        call_PROGRAMTYPE_program(ctx, *index);
        return RX_OK;
      }
    )";

/**
 * Internal method called during enhanced compilation to adjust NON-SWAPPABLE maps
 *
 * Checks for all non-swappable maps and declare them SHARED/extern. This way they
 * can be shared among the two programs (original/swapped).
 * NB:
 *  If some maps is already SHARED/PUBLIC/PINNED, the swapped code will have the "extern"
 *  declaration instead, to reuse the same map.
 *  If maps are declared using more-specific helpers than the classic BPF_TABLE/BPF_QUEUESTACK
 *  (like BPF_ARRAY), the compilation will terminate since still not implemented that part.
 *  For all the other maps, their declaration is changed into SHARED in the "original"
 *  program, and as "extern" in the swap one.
 *
 * @param original_code  the original code to be compiled
 * @param swapped_code   the swap code
 * @param maps_to_swap   the list of maps which need to be swapped
 * @param modified_names the list of maps whose names have been modified
 * @param type           the program type INGRESS/EGRESS
 * @return               true if correctly executed, false otherwise
 */
bool enhancedFixNotSwappableMaps(std::string &original_code, std::string &swapped_code,
                         const std::vector<std::string>& maps_to_swap,
                         std::unordered_map<std::string, std::string> &modified_names,
                         ProgramType type) {
  /*Create a regex Pattern to match all maps not declared swappable*/
  std::smatch match;
  std::string pattern = "BPF_";
  for(auto &m: maps_to_swap)
    pattern += "(?!.+" + m + ")";
  pattern += ".+";

  if (std::regex_search(original_code, match, std::regex(pattern))) {

    /*Retrieving the non-swappable maps that their declaration should be slightly modified in the swapped code*/
    /*Checking if all the non-swappable map are supported as SHARED*/
    for (auto &x: match) {
      auto decl = x.str();

      /*Checking if the map is already "extern" or PINNED*/
      if (decl.find("\"extern\"") != std::string::npos ||
          decl.find("_PINNED") != std::string::npos) {
        continue;
      }

      bool is_table = decl.find("BPF_TABLE") != std::string::npos,
           is_queue = decl.find("BPF_QUEUESTACK") != std::string::npos;
      /*Checking if the map has been declared using the traditional BPF_TABLE or BPF_QUEUESTACK*/
      if(!is_table && !is_queue) {
        // TODO: Change map declaration from helper to standard BPF_TABLE
        // By now if the user has declared the map using an helper
        // (eg. BPF_ARRAY instead of BPF_TABLE) I use the basic compilation
        return false;
      }

      /*Check if map is _SHARED or _PUBLIC that should be marked as extern in swapped code*/
      size_t index;
      if((index=decl.find("_SHARED")) != std::string::npos ||
          (index=decl.find("_PUBLIC")) != std::string::npos){
        std::size_t index_replace_decl = swapped_code.find(decl);
        size_t index_comma_after_name = decl.find(',');
        /*Before erase then insert to avoid that part of declaration stays (if BPF_QUEUESTACK, the name is longer)*/
        swapped_code.erase(index_replace_decl + index,index_comma_after_name - index);
        swapped_code.insert(index_replace_decl + index, "(\"extern\"");
        continue;
      }

      /*Now I know that:
       * - the map has been declared using standard BPF_TABLE* or BPF_QUEUESTACK*
       * - it is a non special type, since already matched
       * - need to set BPF_TABLE_SHARED(..) in original, BPF_TABLE("extern",...) in swapped
       *   and change name accordingly to ingress/egress
       * */

      /*Match the name of the map*/
      if( (is_table && !std::regex_search(decl, match, std::regex(",.*,\\s*(\\w+),.*"))) ||
          (is_queue && !std::regex_search(decl, match, std::regex(",\\s*(\\w+),.*"))) )  {
        return false;
      }

      /*Retrieving indexes to modify codes*/
      auto index_replace_decl_or = original_code.find(decl),
          index_replace_decl_sw = swapped_code.find(decl),
           index_comma_after_name = decl.find(',');

      /*Modifying map name*/
      auto map_name = match[1].str(), map_name_repl = type == ProgramType::INGRESS ?
        map_name + ENHANCED_SWAP_MAP_NAME_FORMAT_INGRESS : map_name + ENHANCED_SWAP_MAP_NAME_FORMAT_EGRESS;

      if(is_table) {
        /* BPF_TABLE = 9 char */
        index_replace_decl_or += 9;
        index_replace_decl_sw += 10;
        index_comma_after_name -= 10;
      } else {
        /* BPF_QUEUESTACK = 14 char */
        index_replace_decl_or += 14;
        index_replace_decl_sw += 15;
        index_comma_after_name -= 15;
      }

      /*Inserting shared in original map and extern in swap map*/
      original_code.insert(index_replace_decl_or, "_SHARED");
      swapped_code.erase(index_replace_decl_sw, index_comma_after_name);
      swapped_code.insert(index_replace_decl_sw, "\"extern\"");

      /*Replacing map name*/
      original_code = std::regex_replace(original_code, std::regex(map_name), map_name_repl);
      swapped_code = std::regex_replace(swapped_code, std::regex(map_name), map_name_repl);

      modified_names.insert({map_name, map_name_repl});
    }
  }
  return true;
}

/**
 * Internal method called to adjust the swappable map declaration.
 *
 * NB: the swappable maps don't need to be declared differently, except if they
 *     belong to any of the following types:
 * - SHARED: the user SHOULD have declared a swappable "extern" map in the other
 *           program type
 * - PUBLIC: the user SHOULD have declared a swappable "extern" map in the other
 *           program type
 * - PINNED: the user MUST have declared both the PINNED_MAP and PINNED_MAP_1
 *          (Risky but better than not supported)
 * - extern: the user SHOULD have declared a swappable SHARED/PUBLIC map in the other
 *           program type
 *
 * @param original_code  the original code to be compiled
 * @param swapped_code   the swap code
 * @param maps_to_swap   the list of maps which need to be swapped
 * @param modified_names the list of maps whose names have been modified
 * @return               true if correctly executed, false otherwise
 */
bool enhancedFixSwappableMaps(std::string &original_code, std::string &swapped_code,
                         const std::vector<std::string>& maps_to_swap,
                         std::unordered_map<std::string, std::string> &modified_names) {
  std::smatch match;

  for(auto &map: maps_to_swap) {
    /*Checking if able to retrieve map declaration*/
    if (!std::regex_search(original_code, match, std::regex("BPF.+(" + map + ").+"))) {
      //Error in regex ?
      return false;
    }
    auto decl = match[0].str();

    /*Checking if special map*/
    if(decl.find("_SHARED") != std::string::npos ||
        decl.find("_PINNED") != std::string::npos ||
        decl.find("_PUBLIC") != std::string::npos ||
        decl.find("\"extern\"") != std::string::npos) {
      auto new_name = map + BASE_SWAP_MAP_NAME_FORMAT;
      swapped_code = std::regex_replace(swapped_code, std::regex(map), new_name);
      modified_names.insert({map, new_name});
    }
    /*All the other maps do not need to be defined differently, since program-scope*/
  }
  return true;
}

/**
 * Internal functions to perform the enhanced compilation.
 *
 * @param original_code the original code to compile
 * @param type          the program type (Ingress/Egress)
 * @param maps_to_swap  the list of maps which need to be swap
 * @param config        the SwapStateConfig to be modified
 * @return              true if correctly performed, false otherwise
 */
bool try_enhance_compilation(std::string original_code, ProgramType type,
                             const std::vector<std::string>& maps_to_swap,
                             SwapStateConfig &config) {
  auto swapped_code = original_code;
  /*Defining map holding the map names which have been modified in the same PROGRAM TYPE*/
  std::unordered_map<std::string, std::string> modified_names;
  /*Defining map holding the map names which have been modified between INGRESS/EGRESS*/
  std::unordered_map<std::string, std::string> modified_global_names;

  if(!enhancedFixNotSwappableMaps(original_code, swapped_code, maps_to_swap, modified_names, type)) {
    return false;
  }
  if(!enhancedFixSwappableMaps(original_code, swapped_code, maps_to_swap, modified_global_names)) {
    return false;
  }

  /*Modify PIVOTING master code according to the program type*/
  auto master_code = std::regex_replace(
      dynmon_swap_master, std::regex("PROGRAMTYPE"),
      type == ProgramType::INGRESS ? "ingress" : "egress");

  config = SwapStateConfig(original_code, swapped_code, CompileType::ENHANCED,
                           master_code, modified_names, modified_global_names);
  return true;
}

/**
 *  Internal functions to perform the base compilation.
 *
 * @param original_code the original code to compile
 * @param maps_to_swap  the list of map which need to be swapped
 * @param config        the SwapStateConfig where all info will be held
 * @return true if correctly executed, false otherwise
 */
bool do_base_compile(std::string original_code,
                     const std::vector<std::string>& maps_to_swap,
                     SwapStateConfig &config) {
  auto swapped_code = original_code;
  std::unordered_map<std::string, std::string> modified_names;
  for(auto &mapName: maps_to_swap) {
    /*Regex used to change from map NAME to NAME_1 */
    std::regex name_sub ("(" + mapName + ")");
    /*Regex to declare also in original program the swapped map and vice-versa.
      This way every reloaded program steals both the maps, otherwise they would not be
      seen anymore*/
    std::regex map_decl ("(BPF_.+" + mapName +".+)");
    std::smatch match1, match2;
    /*Replacing the map name in swap code*/
    swapped_code = std::regex_replace(original_code, name_sub, mapName + BASE_SWAP_MAP_NAME_FORMAT);
    /*Check if able to find declaration in codes*/
    if(std::regex_search(swapped_code, match1, map_decl) && std::regex_search(original_code, match2, map_decl)) {
      /*Retrieving original and swapped declaration and their indexes in codes*/
      auto orig_decl = match2[0].str(), swap_decl = match1[0].str();
      auto orig_index = original_code.find(orig_decl), swap_index = swapped_code.find(swap_decl);
      /*Check if there was an error finding the declaration in the codes*/
      if(orig_index == std::string::npos || swap_index == std::string::npos){
        return false;
      }
      original_code.insert(orig_index, swap_decl + "\n");
      swapped_code.insert(swap_index, orig_decl + "\n");
      /*Map correctly tuned, insert in the list of maps which have the name modified*/
      modified_names.insert({mapName, mapName + BASE_SWAP_MAP_NAME_FORMAT});
    } else {
      return false;
    }
  }
  config = SwapStateConfig(original_code, swapped_code, CompileType::BASE, "", modified_names);
  return true;
}


/*For the doc read SwapCompiler.h*/
void CodeRewriter::compile(std::string &original_code, ProgramType type,
                                      const std::vector<MetricConfigJsonObject> &metricConfigs,
                                      SwapStateConfig &config,
                                      const std::shared_ptr<spdlog::logger>& logger){
  bool is_enabled = false;
  std::vector<std::string> maps_to_swap;
  /*Checking if some map has been declared as SWAP, otherwise no compilation will be performed*/
  for(auto &mc : metricConfigs) {
    if (mc.getExtractionOptions().getSwapOnRead()) {
      maps_to_swap.emplace_back(mc.getMapName());
      is_enabled = true;
    }
  }
  /*If is enabled, try enhanced compilation or basic compilation if it has failed.*/
  if(!is_enabled) {
    config = {};
    logger->info("[Dynmon_CodeRewriter] No map marked as swappable, no rewrites performed");
  } else if(try_enhance_compilation(original_code, type, maps_to_swap, config)) {
    logger->info("[Dynmon_CodeRewriter] Successfully rewritten with ENHANCED tuning");
  } else if(do_base_compile(original_code, maps_to_swap, config)) {
    logger->info("[Dynmon_CodeRewriter] Successfully rewritten with BASE tuning");
  } else {
    config = {};
    logger->info("[Dynmon_CodeRewriter] Error while trying to rewrite the code, no rewrites performed");
  }
}
