include(${PROJECT_SOURCE_DIR}/cmake/LoadFileAsVariable.cmake)

aux_source_directory(model MODEL_SOURCES)
aux_source_directory(api API_SOURCES)

include_directories(model)

# Needed to load files as variables
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(pcn-bridge SHARED
  ${MODEL_SOURCES}
  ${API_SOURCES}
  Bridge.cpp
  BridgeSTP.cpp
  BridgeApi.cpp
  Bridge-lib.cpp)

# load ebpf datapath code in std::string variables
load_file_as_variable(pcn-bridge Bridge_dp_stp.c bridge_code)
load_file_as_variable(pcn-bridge Bridge_dp_no_stp.c bridge_code_no_stp)

target_link_libraries(pcn-bridge
  polycube
  stp
  uuid
  tins
  stp)

# Specify shared library install directory

set(CMAKE_INSTALL_LIBDIR /usr/lib)

install(
    TARGETS
    pcn-bridge
    DESTINATION
    "${CMAKE_INSTALL_LIBDIR}"
)
