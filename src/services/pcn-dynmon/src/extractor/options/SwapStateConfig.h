#pragma once

//The suffix to be added to the original map name when swapped
#define SWAP_MAP_NAME_FORMAT "_1"

/**
 * Container class responsible of holding all the needed data to perform, if needed, program swap
 */
class SwapStateConfig {
 public:
  SwapStateConfig() : swap_code(), orig_code(), swap_enabled(false), has_swapped(false) {};

  /**
   * Method to find out if the actual code in the probe is the swapped one or the "original" one
   * @return
   */
  bool hasSwapped() {
    return has_swapped;
  }

  /**
   * Method to trigger a read, which will define which code has to be reloaded in the probe
   */
  void triggerHasSwapped() {
    has_swapped = !has_swapped;
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
   * Method to return the swap code
   * @return the swap code
   */
  std::string &getSwapCode() {
    return swap_code;
  }

  /**
   * Method to set the swap code
   * @param[code] the code to be set as swap code
   */
  void setSwapCode(std::string &code) {
    swap_code = code;
  }

  /**
   * Method to set the "original" code
   * @param[code] the code to be set as "original"
   */
  void setOrigCode(std::string &code) {
    orig_code = code;
  }

  /**
   * Method to return the "original" code
   * @return the "original" code
   */
  std::string &getOrigCode() {
    return orig_code;
  }

  /**
   * Method to accordingly modify the mapName with the defined suffix
   * @param map_name the name of the map to be modified
   */
  static void formatMapName(std::string &map_name) {
    map_name =  map_name + SWAP_MAP_NAME_FORMAT;
  }

 private:
  std::string swap_code;                    //The swap code accordingly modified
  std::string orig_code;                    //The original code accordingly modified
  bool swap_enabled;                        //Variable to keep track if the program needs to be swapped (at least 1 map with swap enabled)
  bool has_swapped;                         //Variable to keep track if the current program is the swapped one
};