# Install script for directory: /home/pino/polycube/src/libs/bcc/man/man8

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/bcc/man/man8" TYPE FILE FILES
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/argdist.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/bashreadline.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/biolatency.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/biosnoop.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/biotop.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/bitesize.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/bpflist.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/bps.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/btrfsdist.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/btrfsslower.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/cachestat.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/cachetop.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/capable.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/cobjnew.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/cpudist.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/cpuunclaimed.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/criticalstat.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/cthreads.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/dbslower.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/dbstat.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/dcsnoop.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/dcstat.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/deadlock_detector.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/drsnoop.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/execsnoop.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/exitsnoop.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/ext4dist.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/ext4slower.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/filelife.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/fileslower.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/filetop.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/funccount.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/funclatency.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/funcslower.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/gethostlatency.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/hardirqs.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/inject.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/javacalls.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/javaflow.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/javagc.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/javaobjnew.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/javastat.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/javathreads.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/killsnoop.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/llcstat.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/mdflush.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/memleak.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/mountsnoop.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/mysqld_qslower.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/nfsdist.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/nfsslower.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/nodegc.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/nodestat.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/offcputime.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/offwaketime.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/oomkill.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/opensnoop.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/perlcalls.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/perlflow.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/perlstat.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/phpcalls.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/phpflow.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/phpstat.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/pidpersec.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/profile.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/pythoncalls.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/pythonflow.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/pythongc.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/pythonstat.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/reset-trace.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/rubycalls.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/rubyflow.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/rubygc.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/rubyobjnew.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/rubystat.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/runqlat.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/runqlen.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/runqslower.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/shmsnoop.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/slabratetop.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/softirqs.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/spfdsnoop.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/sslsniff.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/stackcount.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/statsnoop.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/syncsnoop.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/syscount.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/tclcalls.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/tclflow.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/tclobjnew.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/tclstat.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/tcpaccept.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/tcpconnect.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/tcpconnlat.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/tcpdrop.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/tcplife.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/tcpretrans.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/tcpstates.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/tcpsubnet.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/tcptop.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/tcptracer.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/tplist.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/trace.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/ttysnoop.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/ucalls.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/uflow.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/ugc.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/uobjnew.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/ustat.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/uthreads.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/vfscount.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/vfsstat.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/wakeuptime.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/xfsdist.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/xfsslower.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/zfsdist.8.gz"
    "/home/pino/polycube/cmake-build-debug/src/libs/bcc/man/man8/zfsslower.8.gz"
    )
endif()

