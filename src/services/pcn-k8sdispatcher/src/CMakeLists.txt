include(${PROJECT_SOURCE_DIR}/cmake/LoadFileAsVariable.cmake)

aux_source_directory(serializer SERIALIZER_SOURCES)
aux_source_directory(api API_SOURCES)
aux_source_directory(base BASE_SOURCES)

include_directories(serializer)

if (NOT DEFINED POLYCUBE_STANDALONE_SERVICE OR POLYCUBE_STANDALONE_SERVICE)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(POLYCUBE libpolycube)
  include_directories(${POLYCUBE_INCLUDE_DIRS})
endif (NOT DEFINED POLYCUBE_STANDALONE_SERVICE OR POLYCUBE_STANDALONE_SERVICE)

# Needed to load files as variables
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(pcn-k8sdispatcher SHARED
    ${SERIALIZER_SOURCES}
    ${API_SOURCES}
    ${BASE_SOURCES}
    K8sdispatcher.cpp
    NodeportRule.cpp
    Ports.cpp
    SessionRule.cpp
    K8sdispatcher-lib.cpp
    Utils.cpp)

# load ebpf datapath code a variable
load_file_as_variable(pcn-k8sdispatcher
    K8sdispatcher_dp.c
    k8sdispatcher_code)

# load datamodel in a variable
load_file_as_variable(pcn-k8sdispatcher
    ../datamodel/k8sdispatcher.yang
    k8sdispatcher_datamodel)

target_link_libraries(pcn-k8sdispatcher ${POLYCUBE_LIBRARIES})

# Specify shared library install directory

set(CMAKE_INSTALL_LIBDIR /usr/lib)

install(
    TARGETS
    pcn-k8sdispatcher
    DESTINATION
    "${CMAKE_INSTALL_LIBDIR}"
)
