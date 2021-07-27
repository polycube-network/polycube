find_package(LibYANG REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/src/libs/bcc/src/cc/libbpf/include/uapi)

option(INSTALL_CLI "Installs the polycube CLI" ON)
option(ENABLE_PCN_IPTABLES "Enables the pcn-iptables" OFF)

# The following is the default option for bcc: embedding LLVM in the lib files.
# However, somebody filled an issue (#335) such as "Option XXXXXX registered more than once"
# Apparently, this could be solved by turning the following flag to ON
option(ENABLE_LLVM_SHARED "Enable linking LLVM as a shared library" OFF)

add_subdirectory(components)
add_subdirectory(libs)

# It fills a list with the name of all enabled services. That list is used by
# cmake in polycubed, for that reason it has to be added before.

add_subdirectory(services)

add_subdirectory(polycubed)
if (INSTALL_CLI)
  add_subdirectory(polycubectl)
endif (INSTALL_CLI)
