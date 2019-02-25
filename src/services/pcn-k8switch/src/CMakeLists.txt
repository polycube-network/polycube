include(${PROJECT_SOURCE_DIR}/cmake/LoadFileAsVariable.cmake)

aux_source_directory(serializer SERIALIZER_SOURCES)
aux_source_directory(api API_SOURCES)
aux_source_directory(default-src SRC_SOURCES)

include_directories(serializer)
include_directories(interface)
include_directories(default-src)

# Needed to load files as variables
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(pcn-k8switch SHARED
  ${SERIALIZER_SOURCES}
  ${API_SOURCES}
  ${SRC_SOURCES}
  FwdTable.cpp
  K8switch.cpp
  Ports.cpp
  Service.cpp
  ServiceBackend.cpp
  K8switch-lib.cpp)

# load ebpf datapath code in std::string variables
load_file_as_variable(pcn-k8switch K8switch_dp.c k8switch_code)

# load datamodel in a variable
load_file_as_variable(pcn-k8switch ../datamodel/k8switch.yang k8switch_datamodel)

target_link_libraries(pcn-k8switch
  polycube
  uuid)

# Specify shared library install directory

set(CMAKE_INSTALL_LIBDIR /usr/lib)

install(
    TARGETS
    pcn-k8switch
    DESTINATION
    "${CMAKE_INSTALL_LIBDIR}"
)
