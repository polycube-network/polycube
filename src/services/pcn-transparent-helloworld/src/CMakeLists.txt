include(${PROJECT_SOURCE_DIR}/cmake/LoadFileAsVariable.cmake)

aux_source_directory(serializer SERIALIZER_SOURCES)
aux_source_directory(api API_SOURCES)
aux_source_directory(default-src SRC_SOURCES)

include_directories(serializer)
include_directories(interface)
include_directories(default-src)

# Needed to load files as variables
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(pcn-transparenthelloworld SHARED
  ${SERIALIZER_SOURCES}
  ${API_SOURCES}
  ${SRC_SOURCES}
  Transparenthelloworld.cpp
  Transparenthelloworld-lib.cpp)

target_link_libraries(pcn-transparenthelloworld
  polycube
  uuid)

# load ebpf datapath code in std::string variables
load_file_as_variable(pcn-transparenthelloworld
  TransparentHelloworld_dp_ingress.c transparent_helloworld_code_ingress)
load_file_as_variable(pcn-transparenthelloworld
  TransparentHelloworld_dp_egress.c transparent_helloworld_code_egress)


# Specify shared library install directory

set(CMAKE_INSTALL_LIBDIR /usr/lib)

install(
    TARGETS
    pcn-transparenthelloworld
    DESTINATION
    "${CMAKE_INSTALL_LIBDIR}"
)
