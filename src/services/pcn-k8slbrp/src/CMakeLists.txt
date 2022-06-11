include(${PROJECT_SOURCE_DIR}/cmake/LoadFileAsVariable.cmake)

aux_source_directory(serializer SERIALIZER_SOURCES)
aux_source_directory(api API_SOURCES)
aux_source_directory(base BASE_SOURCES)

include_directories(serializer)

if (NOT DEFINED POLYCUBE_STANDALONE_SERVICE OR POLYCUBE_STANDALONE_SERVICE)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(POLYCUBE libpolycube)
  include_directories(${POLYCUBE_INCLUDE_DIRS})
endif(NOT DEFINED POLYCUBE_STANDALONE_SERVICE OR POLYCUBE_STANDALONE_SERVICE)

# Needed to load files as variables
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(pcn-k8slbrp SHARED
  ${SERIALIZER_SOURCES}
  ${API_SOURCES}
  ${BASE_SOURCES}
  K8slbrp.cpp
  Ports.cpp
  Service.cpp
  ServiceBackend.cpp
  SrcIpRewrite.cpp
  K8slbrp-lib.cpp)

# load ebpf datapath code a variable
load_file_as_variable(pcn-k8slbrp K8slbrp_dp.c k8slbrp_code)

# load datamodel in a variable
load_file_as_variable(pcn-k8slbrp ../datamodel/k8slbrp.yang k8slbrp_datamodel)

target_link_libraries(pcn-k8slbrp ${POLYCUBE_LIBRARIES} uuid)

# Specify shared library install directory

set(CMAKE_INSTALL_LIBDIR /usr/lib)

install(
    TARGETS
    pcn-k8slbrp
    DESTINATION
    "${CMAKE_INSTALL_LIBDIR}"
)
