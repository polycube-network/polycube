include(${PROJECT_SOURCE_DIR}/cmake/LoadFileAsVariable.cmake)

aux_source_directory(serializer SERIALIZER_SOURCES)
aux_source_directory(api API_SOURCES)
aux_source_directory(base BASE_SOURCES)
aux_source_directory(modules MODULES)

include_directories(serializer)
include_directories(matcher)

if (NOT DEFINED POLYCUBE_STANDALONE_SERVICE OR POLYCUBE_STANDALONE_SERVICE)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(POLYCUBE libpolycube)
  include_directories(${POLYCUBE_INCLUDE_DIRS})
endif(NOT DEFINED POLYCUBE_STANDALONE_SERVICE OR POLYCUBE_STANDALONE_SERVICE)

# Needed to load files as variables
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(pcn-classifier SHARED
  ${SERIALIZER_SOURCES}
  ${API_SOURCES}
  ${BASE_SOURCES}
  ${MODULES}
  TrafficClass.cpp
  Classifier.cpp
  Classifier-lib.cpp
  Utils.cpp)

# load ebpf datapath code a variable
load_file_as_variable(pcn-classifier
  Classifier_dp.c
  classifier_code)
load_file_as_variable(pcn-classifier
  Selector_dp.c
  selector_code)
load_file_as_variable(pcn-classifier
  MatchingTable_dp.c
  matching_table_code)
load_file_as_variable(pcn-classifier
  Matcher_dp.c
  matcher_code)

# load datamodel in a variable
load_file_as_variable(pcn-classifier
  ../datamodel/classifier.yang
  classifier_datamodel)

target_link_libraries(pcn-classifier ${POLYCUBE_LIBRARIES})

# Specify shared library install directory

set(CMAKE_INSTALL_LIBDIR /usr/lib)

install(
    TARGETS
    pcn-classifier
    DESTINATION
    "${CMAKE_INSTALL_LIBDIR}"
)
