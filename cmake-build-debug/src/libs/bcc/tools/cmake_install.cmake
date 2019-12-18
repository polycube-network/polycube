# Install script for directory: /home/pino/polycube/src/libs/bcc/tools

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "argdist" FILES "/home/pino/polycube/src/libs/bcc/tools/argdist.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "bashreadline" FILES "/home/pino/polycube/src/libs/bcc/tools/bashreadline.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "biolatency" FILES "/home/pino/polycube/src/libs/bcc/tools/biolatency.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "biosnoop" FILES "/home/pino/polycube/src/libs/bcc/tools/biosnoop.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "biotop" FILES "/home/pino/polycube/src/libs/bcc/tools/biotop.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "bitesize" FILES "/home/pino/polycube/src/libs/bcc/tools/bitesize.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "bpflist" FILES "/home/pino/polycube/src/libs/bcc/tools/bpflist.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "btrfsdist" FILES "/home/pino/polycube/src/libs/bcc/tools/btrfsdist.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "btrfsslower" FILES "/home/pino/polycube/src/libs/bcc/tools/btrfsslower.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "cachestat" FILES "/home/pino/polycube/src/libs/bcc/tools/cachestat.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "cachetop" FILES "/home/pino/polycube/src/libs/bcc/tools/cachetop.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "capable" FILES "/home/pino/polycube/src/libs/bcc/tools/capable.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "cpudist" FILES "/home/pino/polycube/src/libs/bcc/tools/cpudist.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "cpuunclaimed" FILES "/home/pino/polycube/src/libs/bcc/tools/cpuunclaimed.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "criticalstat" FILES "/home/pino/polycube/src/libs/bcc/tools/criticalstat.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "dbslower" FILES "/home/pino/polycube/src/libs/bcc/tools/dbslower.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "dbstat" FILES "/home/pino/polycube/src/libs/bcc/tools/dbstat.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "dcsnoop" FILES "/home/pino/polycube/src/libs/bcc/tools/dcsnoop.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "dcstat" FILES "/home/pino/polycube/src/libs/bcc/tools/dcstat.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "deadlock" FILES "/home/pino/polycube/src/libs/bcc/tools/deadlock.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "drsnoop" FILES "/home/pino/polycube/src/libs/bcc/tools/drsnoop.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "execsnoop" FILES "/home/pino/polycube/src/libs/bcc/tools/execsnoop.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "exitsnoop" FILES "/home/pino/polycube/src/libs/bcc/tools/exitsnoop.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "ext4dist" FILES "/home/pino/polycube/src/libs/bcc/tools/ext4dist.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "ext4slower" FILES "/home/pino/polycube/src/libs/bcc/tools/ext4slower.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "filelife" FILES "/home/pino/polycube/src/libs/bcc/tools/filelife.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "fileslower" FILES "/home/pino/polycube/src/libs/bcc/tools/fileslower.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "filetop" FILES "/home/pino/polycube/src/libs/bcc/tools/filetop.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "funccount" FILES "/home/pino/polycube/src/libs/bcc/tools/funccount.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "funclatency" FILES "/home/pino/polycube/src/libs/bcc/tools/funclatency.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "funcslower" FILES "/home/pino/polycube/src/libs/bcc/tools/funcslower.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "gethostlatency" FILES "/home/pino/polycube/src/libs/bcc/tools/gethostlatency.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "hardirqs" FILES "/home/pino/polycube/src/libs/bcc/tools/hardirqs.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "inject" FILES "/home/pino/polycube/src/libs/bcc/tools/inject.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "killsnoop" FILES "/home/pino/polycube/src/libs/bcc/tools/killsnoop.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "llcstat" FILES "/home/pino/polycube/src/libs/bcc/tools/llcstat.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "mdflush" FILES "/home/pino/polycube/src/libs/bcc/tools/mdflush.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "memleak" FILES "/home/pino/polycube/src/libs/bcc/tools/memleak.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "mountsnoop" FILES "/home/pino/polycube/src/libs/bcc/tools/mountsnoop.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "mysqld_qslower" FILES "/home/pino/polycube/src/libs/bcc/tools/mysqld_qslower.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "nfsdist" FILES "/home/pino/polycube/src/libs/bcc/tools/nfsdist.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "nfsslower" FILES "/home/pino/polycube/src/libs/bcc/tools/nfsslower.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "offcputime" FILES "/home/pino/polycube/src/libs/bcc/tools/offcputime.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "offwaketime" FILES "/home/pino/polycube/src/libs/bcc/tools/offwaketime.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "oomkill" FILES "/home/pino/polycube/src/libs/bcc/tools/oomkill.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "opensnoop" FILES "/home/pino/polycube/src/libs/bcc/tools/opensnoop.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "pidpersec" FILES "/home/pino/polycube/src/libs/bcc/tools/pidpersec.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "profile" FILES "/home/pino/polycube/src/libs/bcc/tools/profile.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "runqlat" FILES "/home/pino/polycube/src/libs/bcc/tools/runqlat.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "runqlen" FILES "/home/pino/polycube/src/libs/bcc/tools/runqlen.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "runqslower" FILES "/home/pino/polycube/src/libs/bcc/tools/runqslower.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "shmsnoop" FILES "/home/pino/polycube/src/libs/bcc/tools/shmsnoop.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "slabratetop" FILES "/home/pino/polycube/src/libs/bcc/tools/slabratetop.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "sofdsnoop" FILES "/home/pino/polycube/src/libs/bcc/tools/sofdsnoop.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "softirqs" FILES "/home/pino/polycube/src/libs/bcc/tools/softirqs.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "solisten" FILES "/home/pino/polycube/src/libs/bcc/tools/solisten.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "sslsniff" FILES "/home/pino/polycube/src/libs/bcc/tools/sslsniff.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "stackcount" FILES "/home/pino/polycube/src/libs/bcc/tools/stackcount.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "statsnoop" FILES "/home/pino/polycube/src/libs/bcc/tools/statsnoop.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "syncsnoop" FILES "/home/pino/polycube/src/libs/bcc/tools/syncsnoop.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "syscount" FILES "/home/pino/polycube/src/libs/bcc/tools/syscount.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "tcpaccept" FILES "/home/pino/polycube/src/libs/bcc/tools/tcpaccept.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "tcpconnect" FILES "/home/pino/polycube/src/libs/bcc/tools/tcpconnect.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "tcpconnlat" FILES "/home/pino/polycube/src/libs/bcc/tools/tcpconnlat.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "tcpdrop" FILES "/home/pino/polycube/src/libs/bcc/tools/tcpdrop.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "tcplife" FILES "/home/pino/polycube/src/libs/bcc/tools/tcplife.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "tcpretrans" FILES "/home/pino/polycube/src/libs/bcc/tools/tcpretrans.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "tcpstates" FILES "/home/pino/polycube/src/libs/bcc/tools/tcpstates.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "tcpsubnet" FILES "/home/pino/polycube/src/libs/bcc/tools/tcpsubnet.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "tcptop" FILES "/home/pino/polycube/src/libs/bcc/tools/tcptop.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "tcptracer" FILES "/home/pino/polycube/src/libs/bcc/tools/tcptracer.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "tplist" FILES "/home/pino/polycube/src/libs/bcc/tools/tplist.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "trace" FILES "/home/pino/polycube/src/libs/bcc/tools/trace.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "ttysnoop" FILES "/home/pino/polycube/src/libs/bcc/tools/ttysnoop.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "vfscount" FILES "/home/pino/polycube/src/libs/bcc/tools/vfscount.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "vfsstat" FILES "/home/pino/polycube/src/libs/bcc/tools/vfsstat.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "wakeuptime" FILES "/home/pino/polycube/src/libs/bcc/tools/wakeuptime.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "xfsdist" FILES "/home/pino/polycube/src/libs/bcc/tools/xfsdist.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "xfsslower" FILES "/home/pino/polycube/src/libs/bcc/tools/xfsslower.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "zfsdist" FILES "/home/pino/polycube/src/libs/bcc/tools/zfsdist.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "zfsslower" FILES "/home/pino/polycube/src/libs/bcc/tools/zfsslower.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "cobjnew" FILES "/home/pino/polycube/src/libs/bcc/tools/cobjnew.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/cobjnew" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/uobjnew -l c \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "javacalls" FILES "/home/pino/polycube/src/libs/bcc/tools/javacalls.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/javacalls" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/ucalls -l java \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "javaflow" FILES "/home/pino/polycube/src/libs/bcc/tools/javaflow.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/javaflow" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/uflow -l java \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "javagc" FILES "/home/pino/polycube/src/libs/bcc/tools/javagc.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/javagc" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/ugc -l java \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "javaobjnew" FILES "/home/pino/polycube/src/libs/bcc/tools/javaobjnew.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/javaobjnew" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/uobjnew -l java \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "javastat" FILES "/home/pino/polycube/src/libs/bcc/tools/javastat.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/javastat" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/ustat -l java \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "javathreads" FILES "/home/pino/polycube/src/libs/bcc/tools/javathreads.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/javathreads" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/uthreads -l java \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "nodegc" FILES "/home/pino/polycube/src/libs/bcc/tools/nodegc.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/nodegc" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/ugc -l node \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "nodestat" FILES "/home/pino/polycube/src/libs/bcc/tools/nodestat.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/nodestat" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/ustat -l node \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "perlcalls" FILES "/home/pino/polycube/src/libs/bcc/tools/perlcalls.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/perlcalls" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/ucalls -l perl \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "perlflow" FILES "/home/pino/polycube/src/libs/bcc/tools/perlflow.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/perlflow" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/uflow -l perl \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "perlstat" FILES "/home/pino/polycube/src/libs/bcc/tools/perlstat.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/perlstat" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/ustat -l perl \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "phpcalls" FILES "/home/pino/polycube/src/libs/bcc/tools/phpcalls.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/phpcalls" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/ucalls -l php \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "phpflow" FILES "/home/pino/polycube/src/libs/bcc/tools/phpflow.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/phpflow" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/uflow -l php \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "phpstat" FILES "/home/pino/polycube/src/libs/bcc/tools/phpstat.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/phpstat" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/ustat -l php \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "pythoncalls" FILES "/home/pino/polycube/src/libs/bcc/tools/pythoncalls.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/pythoncalls" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/ucalls -l python \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "pythonflow" FILES "/home/pino/polycube/src/libs/bcc/tools/pythonflow.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/pythonflow" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/uflow -l python \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "pythongc" FILES "/home/pino/polycube/src/libs/bcc/tools/pythongc.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/pythongc" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/ugc -l python \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "pythonstat" FILES "/home/pino/polycube/src/libs/bcc/tools/pythonstat.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/pythonstat" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/ustat -l python \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "reset-trace" FILES "/home/pino/polycube/src/libs/bcc/tools/reset-trace.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "rubycalls" FILES "/home/pino/polycube/src/libs/bcc/tools/rubycalls.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/rubycalls" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/ucalls -l ruby \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "rubyflow" FILES "/home/pino/polycube/src/libs/bcc/tools/rubyflow.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/rubyflow" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/uflow -l ruby \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "rubygc" FILES "/home/pino/polycube/src/libs/bcc/tools/rubygc.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/rubygc" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/ugc -l ruby \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "rubyobjnew" FILES "/home/pino/polycube/src/libs/bcc/tools/rubyobjnew.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/rubyobjnew" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/uobjnew -l ruby \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "rubystat" FILES "/home/pino/polycube/src/libs/bcc/tools/rubystat.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/rubystat" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/ustat -l ruby \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "tclcalls" FILES "/home/pino/polycube/src/libs/bcc/tools/tclcalls.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/tclcalls" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/ucalls -l tcl \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "tclflow" FILES "/home/pino/polycube/src/libs/bcc/tools/tclflow.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/tclflow" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/uflow -l tcl \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "tclobjnew" FILES "/home/pino/polycube/src/libs/bcc/tools/tclobjnew.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/tclobjnew" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/uobjnew -l tcl \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE PROGRAM RENAME "tclstat" FILES "/home/pino/polycube/src/libs/bcc/tools/tclstat.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(WRITE "$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/share/bcc/tools/tclstat" "#!/bin/bash
lib=$(dirname $0)/lib
$lib/ustat -l tcl \"$@\"
")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools" TYPE FILE FILES "/home/pino/polycube/src/libs/bcc/tools/deadlock.c")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/tools/doc" TYPE FILE FILES
    "/home/pino/polycube/src/libs/bcc/tools/argdist_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/bashreadline_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/biolatency_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/biosnoop_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/biotop_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/bitesize_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/bpflist_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/btrfsdist_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/btrfsslower_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/cachestat_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/cachetop_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/capable_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/cobjnew_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/cpudist_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/cpuunclaimed_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/criticalstat_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/cthreads_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/dbslower_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/dbstat_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/dcsnoop_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/dcstat_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/deadlock_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/drsnoop_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/execsnoop_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/exitsnoop_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/ext4dist_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/ext4slower_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/filelife_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/fileslower_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/filetop_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/funccount_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/funclatency_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/funcslower_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/gethostlatency_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/hardirqs_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/inject_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/javacalls_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/javaflow_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/javagc_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/javaobjnew_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/javastat_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/javathreads_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/killsnoop_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/llcstat_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/mdflush_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/memleak_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/mountsnoop_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/mysqld_qslower_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/nfsdist_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/nfsslower_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/nodegc_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/nodestat_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/offcputime_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/offwaketime_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/oomkill_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/opensnoop_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/perlcalls_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/perlflow_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/perlstat_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/phpcalls_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/phpflow_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/phpstat_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/pidpersec_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/profile_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/pythoncalls_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/pythonflow_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/pythongc_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/pythonstat_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/reset-trace_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/rubycalls_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/rubyflow_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/rubygc_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/rubyobjnew_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/rubystat_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/runqlat_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/runqlen_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/runqslower_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/shmsnoop_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/slabratetop_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/sofdsnoop_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/softirqs_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/solisten_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/sslsniff_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/stackcount_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/statsnoop_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/syncsnoop_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/syscount_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/tclcalls_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/tclflow_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/tclobjnew_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/tclstat_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/tcpaccept_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/tcpconnect_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/tcpconnlat_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/tcpdrop_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/tcplife_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/tcpretrans_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/tcpstates_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/tcpsubnet_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/tcptop_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/tcptracer_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/tplist_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/trace_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/ttysnoop_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/vfscount_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/vfsstat_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/wakeuptime_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/xfsdist_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/xfsslower_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/zfsdist_example.txt"
    "/home/pino/polycube/src/libs/bcc/tools/zfsslower_example.txt"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.

endif()

