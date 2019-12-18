find_package(PkgConfig REQUIRED)

find_package(Pistache CONFIG REQUIRED)

find_package(prometheus-cpp CONFIG REQUIRED)

find_package(OpenSSL REQUIRED)
include_directories(OPENSSL_INCLUDE_DIR)

pkg_check_modules(NL3 libnl-3.0)
pkg_check_modules(NL3GENL libnl-genl-3.0)
pkg_check_modules(NL3ROUTE libnl-route-3.0)

pkg_check_modules(TINS libtins)

include_directories(${NL3_INCLUDE_DIRS})
include_directories(${TINS_INCLUDE_DIRS})

include_directories(${CMAKE_SOURCE_DIR}/src/libs/polycube/include/)
include_directories(${CMAKE_SOURCE_DIR}/src/libs/spdlog/)
include_directories(${CMAKE_SOURCE_DIR}/src/libs/viface/)
include_directories(${CMAKE_SOURCE_DIR}/src/libs/bcc/src/cc/)
# include_directories(${CMAKE_SOURCE_DIR}/src/libs/prometheus-cpp/core/include)
# include_directories(${CMAKE_SOURCE_DIR}/src/libs/prometheus-cpp/pull)
# include_directories(${CMAKE_SOURCE_DIR}/src/libs/prometheus-cpp/push)

GET_PROPERTY(LOAD_SERVICES GLOBAL PROPERTY LOAD_SERVICES_)

configure_file(load_services.cpp.in load_services.cpp)
configure_file(version.cpp.in version.cpp)

# this is needed for load_services.cpp that is located under a different folder
include_directories(.)

add_subdirectory(server)

set(polycubed_sources
  bcc_mutex.cpp
  config.cpp
  polycubed.cpp
  base_model.cpp
  cube_factory_impl.cpp
  management_lib.cpp
  polycubed_core.cpp
  rest_server.cpp
  peer_iface.cpp
  port.cpp
  port_tc.cpp
  port_xdp.cpp
  base_cube.cpp
  cube.cpp
  cube_tc.cpp
  cube_xdp.cpp
  transparent_cube.cpp
  transparent_cube_tc.cpp
  transparent_cube_xdp.cpp
  controller.cpp
  extiface.cpp
  extiface_tc.cpp
  extiface_xdp.cpp
  service_controller.cpp
  patchpanel.cpp
  datapath_log.cpp
  id_generator.cpp
  utils/extiface_info.cpp
  utils/ns.cpp
  utils/veth.cpp
  utils/netlink.cpp
  utils/utils.cpp
  cubes_dump.cpp
  ${server_sources}
  ${CMAKE_CURRENT_BINARY_DIR}/load_services.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/version.cpp)

add_executable(polycubed
  ${polycubed_sources})

set(polycubed_libraries
  polycube
  uuid
  viface
  dl
  pistache
  prometheus-cpp-core
  prometheus-cpp-pull
  prometheus-cpp-push
  pthread
  bcc-static
  yang
  xpath
  ${OPENSSL_LIBRARIES}
  ${NL3_LIBRARIES}
  ${NL3ROUTE_LIBRARIES}
  ${NL3GENL_LIBRARIES}
  ${TINS_LIBRARIES}
)

target_link_libraries(polycubed
        ${polycubed_libraries})

# for now install the binary directly, later on we could create a folder with
# the binary and all the resources
install (TARGETS polycubed DESTINATION bin)

# create /var/log/polycube and give permissions to this user
install(CODE
"execute_process( COMMAND /bin/sh -c \"mkdir -p /var/log/polycube && chown $ENV{USER} /var/log/polycube \")")

