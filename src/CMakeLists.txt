find_package(LibYANG REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/src/libs/bcc/src/cc/libbpf/include/uapi)

option(INSTALL_CLI "installs the polycube CLI" ON)
option(ENABLE_PCN_IPTABLES "enables the pcn-iptables" OFF)
option(ENABLE_LLVM_SHARED "Enable linking LLVM as a shared library" ON)

add_subdirectory(components)
add_subdirectory(libs)

# it fills a list with the name of all enabled services, that list is used by
# the cmake in polycubed, for that reason it has to be added before.

add_subdirectory(services)

add_subdirectory(polycubed)
if (INSTALL_CLI)
  add_subdirectory(polycubectl)
endif (INSTALL_CLI)
