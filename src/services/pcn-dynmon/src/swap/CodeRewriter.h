#pragma once

#include <string>

#include "../models/MetricConfig.h"
#include "./SwapStateConfig.h"

using namespace polycube::service;

namespace polycube {
namespace service {
enum class ProgramType;

namespace CodeRewriter {
  /*
   * Enum class to define the Compilation Type:
   * - None               => there was no need to optimize/compile the injected code
   * - PROGRAM_RELOAD     => the codes will be alternatively swapped, replacing the current active one
   *                         (every GET requires almost 400ms seconds to reload the code in the probe)
   * - PROGRAM_INDEX_SWAP => the codes are both injected as two different programs and they will be
   *                          managed by a Master code which will pivot them, calling the right
   *                          alternatively.
   *                          (every GET requires almost 4ms to change the pivoting index)
   */
  enum class CompileType { NONE, PROGRAM_RELOAD, PROGRAM_INDEX_SWAP};

  /*The Master/Pivoting program map name where to store the current active program when CompileType=PROGRAM_INDEX_SWAP*/
  const char SWAP_MASTER_INDEX_MAP[] = "INTERNAL_PROGRAM_INDEX";

  /**
   * Method to compile/optimize user injected code according to the parameter he has inserted.
   * Firstly, it is checked if there's the need of performing optimization.
   * If some map has been declared as swappable, then will be performed the following
   * optimization in cascade if some of them fails:
   * - PROGRAM_INDEX_SWAP: a master pivoting code will control which program to call when new
   *            packets arrive; new parallel maps will be declared and for those which
   *            have not been declared swappable, a SHARED map will be used among the
   *            two programs.
   *            Read time: 5ms (index change) + time to read map
   * - PROGRAM_RELOAD:    two codes will be generated and they will alternatively be loaded into
   *            the probe. Every swappable map has a modified name in the second program,
   *            while the non-swappable will be shared thanks to program map stealing.
   *            Read time: 400ms (program reload) + time to read map
   *
   * @param original_code
   * @param type
   * @param metricConfigs
   * @param config
   * @param logger
   */
  extern void compile(std::string &original_code, ProgramType type,
                   const std::vector<MetricConfigJsonObject> &metricConfigs,
                                 SwapStateConfig &config,
                                 const std::shared_ptr<spdlog::logger>& logger);
}}}