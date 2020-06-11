#pragma once

#include <regex>

//The suffix to be added to the original map name when swapped
#define SWAP_MAP_NAME_FORMAT "_1"

/**
 * Container class responsible of holding all the needed data to perform, if needed, program swap
 */
class SwapConfig {
 public:
  SwapConfig() : swap_code(), orig_code(), swap_enabled(false), has_swapped(false) {};

  /**
   * Method to check if the current loaded code is the swap one
   * @return true if loaded code is the swapped one, false otherwise
   */
  bool hasSwapped() {
    return has_swapped;
  }

  /**
   * Method to find out if the swap functionality is enabled
   * @return true if swap enabled, false otherwise
   */
  bool isSwapEnabled() {
    return swap_enabled;
  }

  /**
   * Method to enable the swap functionality
   */
  void enableSwap() {
    swap_enabled = true;
  }

  /**
   * Method to set the codes to be alternatively swapped
   * @param[code] the code to be set as swap code
   */
  void setCodes(std::string &orig, std::string &swap) {
    orig_code = orig;
    swap_code = swap;
  }

  /**
   * Method to return the return the next code to load.
   * @return the "original" code
   */
  std::string &getCodeToLoad() {
    has_swapped = !has_swapped;
    return has_swapped ? swap_code : orig_code;
  }

  /**
   * Method to accordingly modify the mapName with the defined suffix
   * @param map_name the name of the map to be modified
   */
  static void formatMapName(std::string &map_name) {
    map_name =  map_name + SWAP_MAP_NAME_FORMAT;
  }

  /**
   * Method to modify both original and swapped code accordingly to the mapName to be swapped.
   * Eg. if BPF_ARRAY(JOHN_DOE, u64, 1) needs to be swapped, then
   *    - in the original code will be inserted also BPF_ARRAY(JOHN_DOE_1, u64, 1)
   *    - in the swapped code all JOHN_DOE references will become JOHN_DOE_1
   *    - in the swapped code will be inserted also BPF_ARRAY(JOHN_DOE, u64, 1)
   * Those insertions are vital, since when reloading a program the new one must keep
   * the original/swapped map alive, otherwise if it would not declare it the map would
   * be destroyed, thus not able to be read.
   *
   * @param[original_code] the original code to be modified
   * @param[swapped_code] the swapped code to be modified
   * @param[mapName] the name of the map to be swapped
   */
  static void formatCodeToSwap(std::string &original_code, std::string &swapped_code,
                                       const std::string& mapName) {
    std::regex name_sub ("(" + mapName + ")");   // to change from name to name_1
    std::regex map_decl ("(BPF_.+" + mapName +".+)"); // to declare the map the same, otherwise not accessible
    std::smatch match1, match2;
    swapped_code = std::regex_replace(original_code, name_sub, mapName + SWAP_MAP_NAME_FORMAT);
    if(std::regex_search(swapped_code, match1, map_decl) && std::regex_search(original_code, match2, map_decl)) {
      std::string orig_decl = match2[0].str();
      std::string swap_decl = match1[0].str();
      std::size_t orig_index = original_code.find(orig_decl);   //index must exists since regex has matched
      std::size_t swap_index = swapped_code.find(swap_decl);    //index must exists since regex has matched
      original_code.insert(orig_index, swap_decl + "\n");
      swapped_code.insert(swap_index, orig_decl + "\n");
    } else
      throw std::runtime_error("Fix regex search method - formatCodeToSwap()");
  }

 private:
  std::string swap_code;                    //The swap code accordingly modified
  std::string orig_code;                    //The original code accordingly modified
  bool swap_enabled;                        //Variable to keep track if the program needs to be swapped (at least 1 map with swap enabled)
  bool has_swapped;                         //Variable to keep track if the current program is the swapped one
};