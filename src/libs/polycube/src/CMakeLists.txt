include_directories(${CMAKE_SOURCE_DIR}/src/polycubed/src)
include_directories(${CMAKE_SOURCE_DIR}/src/libs/polycube/include)
include_directories(${CMAKE_SOURCE_DIR}/src/libs/spdlog)
include_directories(${CMAKE_SOURCE_DIR}/src/libs/bcc/src/cc/)

add_definitions(-DGUID_LIBUUID)

add_library(polycube STATIC
  base_cube.cpp
  transparent_cube.cpp
  port.cpp
  table.cpp
  http_router.cpp
  http.cpp
  http_defs.cpp
  utils.cpp
  guid.cpp
  common.cpp)

set_property(TARGET polycube PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(polycube tins)

# Install libpolycube, it is used by services that are compiled standalone
set(CMAKE_INSTALL_LIBDIR /usr/lib)

install(
    TARGETS
    polycube
    DESTINATION
    "${CMAKE_INSTALL_LIBDIR}"
)
