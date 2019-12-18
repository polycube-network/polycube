# Install script for directory: /home/pino/polycube/src/services

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/polycube" TYPE DIRECTORY FILES "/home/pino/polycube/src/services/datamodel-common")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/pino/polycube/cmake-build-debug/src/services/pcn-bridge/cmake_install.cmake")
  include("/home/pino/polycube/cmake-build-debug/src/services/pcn-ddosmitigator/cmake_install.cmake")
  include("/home/pino/polycube/cmake-build-debug/src/services/pcn-firewall/cmake_install.cmake")
  include("/home/pino/polycube/cmake-build-debug/src/services/pcn-helloworld/cmake_install.cmake")
  include("/home/pino/polycube/cmake-build-debug/src/services/pcn-k8switch/cmake_install.cmake")
  include("/home/pino/polycube/cmake-build-debug/src/services/pcn-k8sfilter/cmake_install.cmake")
  include("/home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-dsr/cmake_install.cmake")
  include("/home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/cmake_install.cmake")
  include("/home/pino/polycube/cmake-build-debug/src/services/pcn-nat/cmake_install.cmake")
  include("/home/pino/polycube/cmake-build-debug/src/services/pcn-pbforwarder/cmake_install.cmake")
  include("/home/pino/polycube/cmake-build-debug/src/services/pcn-router/cmake_install.cmake")
  include("/home/pino/polycube/cmake-build-debug/src/services/pcn-simplebridge/cmake_install.cmake")
  include("/home/pino/polycube/cmake-build-debug/src/services/pcn-simpleforwarder/cmake_install.cmake")
  include("/home/pino/polycube/cmake-build-debug/src/services/pcn-iptables/cmake_install.cmake")
  include("/home/pino/polycube/cmake-build-debug/src/services/pcn-transparent-helloworld/cmake_install.cmake")
  include("/home/pino/polycube/cmake-build-debug/src/services/pcn-synflood/cmake_install.cmake")
  include("/home/pino/polycube/cmake-build-debug/src/services/pcn-packetcapture/cmake_install.cmake")

endif()

