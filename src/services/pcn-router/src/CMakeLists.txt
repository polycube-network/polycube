include(${PROJECT_SOURCE_DIR}/cmake/LoadFileAsVariable.cmake)

aux_source_directory(serializer SERIALIZER_SOURCES)
aux_source_directory(api API_SOURCES)
aux_source_directory(default-src SRC_SOURCES)

include_directories(serializer)
include_directories(interface)
include_directories(default-src)

# Needed to load files as variables
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(pcn-router SHARED
  ${SERIALIZER_SOURCES}
  ${API_SOURCES}
  ${SRC_SOURCES}
  ArpEntry.cpp
  Ports.cpp
  PortsSecondaryip.cpp
  Route.cpp
  Router.cpp
  Router-lib.cpp
  CircularBuffer.cpp
  UtilityMethods.cpp)

# load ebpf datapath code in std::string variables
load_file_as_variable(pcn-router Router_dp.c router_code)

target_link_libraries(pcn-router
  polycube
  tins
  uuid)

# Specify shared library install directory

set(CMAKE_INSTALL_LIBDIR /usr/lib)

install(
    TARGETS
    pcn-router
    DESTINATION
    "${CMAKE_INSTALL_LIBDIR}"
)
