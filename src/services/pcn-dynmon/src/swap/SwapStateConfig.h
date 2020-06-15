#pragma once

#include <string>
#include <array>
#include <unordered_map>


namespace polycube{
  namespace service {
    namespace SwapCompiler {
      enum class CompileType;
}}}

using namespace polycube::service::SwapCompiler;

/**
 * Container class responsible of holding all data concerning the program
 * swap state and needs
 */
class SwapStateConfig {
 public:
  SwapStateConfig();
  /**
   * Constructor used by SwapCompiler once finished compiling the code
   *
   * @param original_code the "original" tuned code
   * @param swapped_code  the swap tuned code
   * @param compileType   the type of the compilation performed
   * @param master_code   the pivoting code to inject in case of an ENHANCED compilation
   * @param names         an hash map containing the new names of those maps which have changed
   * @param global_names  an hash map holding the new names of those maps shared among
   *                      INGRESS and EGRESS for whose was enabled SWAP-ON-READ
   */
  SwapStateConfig(std::string original_code, std::string swapped_code,
                  CompileType compileType, std::string master_code = "",
                  std::unordered_map<std::string,std::string> names = {},
                  std::unordered_map<std::string,std::string> global_names = {});

  /**
   * Method to trigger a read. Modify accordingly the next program index
   *
   * @return the next index to be used to call eBPF program
   */
  int triggerRead() {
    current_program_index = (current_program_index % 2) + 1;
    return current_program_index;
  }

  /**
   * Method to return the compilation type performed
   *
   * @return the compilation type
   */
  CompileType getCompileType() {
    return compileType;
  }

  /**
   * Method to return the return the next code to load.
   *
   * @return the "original" code
   */
  const std::string &getCodeToLoad() {
    return codes[current_program_index];
  }

  /**
   * Method to return the "original" code
   *
   * @return the "original" code
   */
  const std::string &getOriginalCode() {
    return codes[1];
  }

  /**
   * Method to return the swap code
   *
   * @return the swap code
   */
  const std::string &getSwappedCode() {
    return codes[2];
  }

  /**
   * Method to return the Master pivoting code
   *
   * @return the master pivoting code
   */
  const std::string &getMasterCode() {
    return codes[0];
  }

  /**
   * Method to return the index of the program where to read the map from.
   * This method also modifies the mapName if needed.
   *
   * @return the index of the program which is not loaded (where I can/should read the maps)
   */
  int getProgramIndexAndMapNameToRead(std::string &mapName);

 private:
  /** Variable to keep track of the current program index*/
  int current_program_index;
  /** Array to store the optimized codes */
  std::array<std::string, 3> codes;
  /** Variable to keep track of the map tuned name if any */
  std::unordered_map<std::string, std::string> enhanced_names;
  /** Variable to keep track of the map tuned name between EGRESS-INGRESS if any */
  std::unordered_map<std::string, std::string> enhanced_global_names;
  /** Variable to keep track of the compilation type */
  CompileType compileType;
};