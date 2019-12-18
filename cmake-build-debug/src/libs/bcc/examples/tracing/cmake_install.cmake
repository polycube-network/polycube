# Install script for directory: /home/pino/polycube/src/libs/bcc/examples/tracing

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/examples/tracing" TYPE PROGRAM FILES
    "/home/pino/polycube/src/libs/bcc/examples/tracing/bitehist.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/dddos.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/disksnoop.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/hello_fields.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/hello_perf_output.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/kvm_hypercall.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/mallocstacks.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/mysqld_query.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/nodejs_http_server.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/stack_buildid_example.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/stacksnoop.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/strlen_count.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/strlen_hist.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/strlen_snoop.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/sync_timing.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/task_switch.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/tcpv4connect.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/trace_fields.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/trace_perf_output.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/urandomread-explicit.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/urandomread.py"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/vfsreadlat.py"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/examples/tracing" TYPE FILE FILES
    "/home/pino/polycube/src/libs/bcc/examples/tracing/task_switch.c"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/vfsreadlat.c"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/examples/tracing" TYPE FILE FILES
    "/home/pino/polycube/src/libs/bcc/examples/tracing/CMakeLists.txt"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/bitehist_example.txt"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/dddos_example.txt"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/disksnoop_example.txt"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/kvm_hypercall.txt"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/mysqld_query_example.txt"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/nodejs_http_server_example.txt"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/stacksnoop_example.txt"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/tcpv4connect_example.txt"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/urandomread_example.txt"
    "/home/pino/polycube/src/libs/bcc/examples/tracing/vfsreadlat_example.txt"
    )
endif()

