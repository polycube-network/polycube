# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.15

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/clion-2019.2.4/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /opt/clion-2019.2.4/bin/cmake/linux/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/pino/polycube

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/pino/polycube/cmake-build-debug

# Include any dependencies generated for this target.
include src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/depend.make

# Include the progress variables for this target.
include src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/progress.make

# Include the compile flags for this target's objects.
include src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/flags.make

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/JsonObjectBase.cpp.o: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/flags.make
src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/JsonObjectBase.cpp.o: ../src/services/pcn-loadbalancer-rp/src/serializer/JsonObjectBase.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/JsonObjectBase.cpp.o"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/pcn-lbrp.dir/serializer/JsonObjectBase.cpp.o -c /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/serializer/JsonObjectBase.cpp

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/JsonObjectBase.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/pcn-lbrp.dir/serializer/JsonObjectBase.cpp.i"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/serializer/JsonObjectBase.cpp > CMakeFiles/pcn-lbrp.dir/serializer/JsonObjectBase.cpp.i

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/JsonObjectBase.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/pcn-lbrp.dir/serializer/JsonObjectBase.cpp.s"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/serializer/JsonObjectBase.cpp -o CMakeFiles/pcn-lbrp.dir/serializer/JsonObjectBase.cpp.s

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/LbrpJsonObject.cpp.o: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/flags.make
src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/LbrpJsonObject.cpp.o: ../src/services/pcn-loadbalancer-rp/src/serializer/LbrpJsonObject.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/LbrpJsonObject.cpp.o"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/pcn-lbrp.dir/serializer/LbrpJsonObject.cpp.o -c /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/serializer/LbrpJsonObject.cpp

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/LbrpJsonObject.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/pcn-lbrp.dir/serializer/LbrpJsonObject.cpp.i"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/serializer/LbrpJsonObject.cpp > CMakeFiles/pcn-lbrp.dir/serializer/LbrpJsonObject.cpp.i

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/LbrpJsonObject.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/pcn-lbrp.dir/serializer/LbrpJsonObject.cpp.s"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/serializer/LbrpJsonObject.cpp -o CMakeFiles/pcn-lbrp.dir/serializer/LbrpJsonObject.cpp.s

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/PortsJsonObject.cpp.o: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/flags.make
src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/PortsJsonObject.cpp.o: ../src/services/pcn-loadbalancer-rp/src/serializer/PortsJsonObject.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/PortsJsonObject.cpp.o"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/pcn-lbrp.dir/serializer/PortsJsonObject.cpp.o -c /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/serializer/PortsJsonObject.cpp

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/PortsJsonObject.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/pcn-lbrp.dir/serializer/PortsJsonObject.cpp.i"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/serializer/PortsJsonObject.cpp > CMakeFiles/pcn-lbrp.dir/serializer/PortsJsonObject.cpp.i

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/PortsJsonObject.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/pcn-lbrp.dir/serializer/PortsJsonObject.cpp.s"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/serializer/PortsJsonObject.cpp -o CMakeFiles/pcn-lbrp.dir/serializer/PortsJsonObject.cpp.s

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/ServiceBackendJsonObject.cpp.o: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/flags.make
src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/ServiceBackendJsonObject.cpp.o: ../src/services/pcn-loadbalancer-rp/src/serializer/ServiceBackendJsonObject.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/ServiceBackendJsonObject.cpp.o"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/pcn-lbrp.dir/serializer/ServiceBackendJsonObject.cpp.o -c /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/serializer/ServiceBackendJsonObject.cpp

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/ServiceBackendJsonObject.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/pcn-lbrp.dir/serializer/ServiceBackendJsonObject.cpp.i"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/serializer/ServiceBackendJsonObject.cpp > CMakeFiles/pcn-lbrp.dir/serializer/ServiceBackendJsonObject.cpp.i

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/ServiceBackendJsonObject.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/pcn-lbrp.dir/serializer/ServiceBackendJsonObject.cpp.s"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/serializer/ServiceBackendJsonObject.cpp -o CMakeFiles/pcn-lbrp.dir/serializer/ServiceBackendJsonObject.cpp.s

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/ServiceJsonObject.cpp.o: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/flags.make
src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/ServiceJsonObject.cpp.o: ../src/services/pcn-loadbalancer-rp/src/serializer/ServiceJsonObject.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/ServiceJsonObject.cpp.o"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/pcn-lbrp.dir/serializer/ServiceJsonObject.cpp.o -c /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/serializer/ServiceJsonObject.cpp

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/ServiceJsonObject.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/pcn-lbrp.dir/serializer/ServiceJsonObject.cpp.i"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/serializer/ServiceJsonObject.cpp > CMakeFiles/pcn-lbrp.dir/serializer/ServiceJsonObject.cpp.i

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/ServiceJsonObject.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/pcn-lbrp.dir/serializer/ServiceJsonObject.cpp.s"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/serializer/ServiceJsonObject.cpp -o CMakeFiles/pcn-lbrp.dir/serializer/ServiceJsonObject.cpp.s

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/SrcIpRewriteJsonObject.cpp.o: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/flags.make
src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/SrcIpRewriteJsonObject.cpp.o: ../src/services/pcn-loadbalancer-rp/src/serializer/SrcIpRewriteJsonObject.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/SrcIpRewriteJsonObject.cpp.o"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/pcn-lbrp.dir/serializer/SrcIpRewriteJsonObject.cpp.o -c /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/serializer/SrcIpRewriteJsonObject.cpp

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/SrcIpRewriteJsonObject.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/pcn-lbrp.dir/serializer/SrcIpRewriteJsonObject.cpp.i"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/serializer/SrcIpRewriteJsonObject.cpp > CMakeFiles/pcn-lbrp.dir/serializer/SrcIpRewriteJsonObject.cpp.i

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/SrcIpRewriteJsonObject.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/pcn-lbrp.dir/serializer/SrcIpRewriteJsonObject.cpp.s"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/serializer/SrcIpRewriteJsonObject.cpp -o CMakeFiles/pcn-lbrp.dir/serializer/SrcIpRewriteJsonObject.cpp.s

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/api/LbrpApi.cpp.o: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/flags.make
src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/api/LbrpApi.cpp.o: ../src/services/pcn-loadbalancer-rp/src/api/LbrpApi.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building CXX object src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/api/LbrpApi.cpp.o"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/pcn-lbrp.dir/api/LbrpApi.cpp.o -c /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/api/LbrpApi.cpp

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/api/LbrpApi.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/pcn-lbrp.dir/api/LbrpApi.cpp.i"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/api/LbrpApi.cpp > CMakeFiles/pcn-lbrp.dir/api/LbrpApi.cpp.i

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/api/LbrpApi.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/pcn-lbrp.dir/api/LbrpApi.cpp.s"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/api/LbrpApi.cpp -o CMakeFiles/pcn-lbrp.dir/api/LbrpApi.cpp.s

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/api/LbrpApiImpl.cpp.o: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/flags.make
src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/api/LbrpApiImpl.cpp.o: ../src/services/pcn-loadbalancer-rp/src/api/LbrpApiImpl.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building CXX object src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/api/LbrpApiImpl.cpp.o"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/pcn-lbrp.dir/api/LbrpApiImpl.cpp.o -c /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/api/LbrpApiImpl.cpp

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/api/LbrpApiImpl.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/pcn-lbrp.dir/api/LbrpApiImpl.cpp.i"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/api/LbrpApiImpl.cpp > CMakeFiles/pcn-lbrp.dir/api/LbrpApiImpl.cpp.i

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/api/LbrpApiImpl.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/pcn-lbrp.dir/api/LbrpApiImpl.cpp.s"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/api/LbrpApiImpl.cpp -o CMakeFiles/pcn-lbrp.dir/api/LbrpApiImpl.cpp.s

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Lbrp.cpp.o: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/flags.make
src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Lbrp.cpp.o: ../src/services/pcn-loadbalancer-rp/src/Lbrp.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building CXX object src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Lbrp.cpp.o"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/pcn-lbrp.dir/Lbrp.cpp.o -c /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/Lbrp.cpp

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Lbrp.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/pcn-lbrp.dir/Lbrp.cpp.i"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/Lbrp.cpp > CMakeFiles/pcn-lbrp.dir/Lbrp.cpp.i

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Lbrp.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/pcn-lbrp.dir/Lbrp.cpp.s"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/Lbrp.cpp -o CMakeFiles/pcn-lbrp.dir/Lbrp.cpp.s

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Ports.cpp.o: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/flags.make
src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Ports.cpp.o: ../src/services/pcn-loadbalancer-rp/src/Ports.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Building CXX object src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Ports.cpp.o"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/pcn-lbrp.dir/Ports.cpp.o -c /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/Ports.cpp

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Ports.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/pcn-lbrp.dir/Ports.cpp.i"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/Ports.cpp > CMakeFiles/pcn-lbrp.dir/Ports.cpp.i

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Ports.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/pcn-lbrp.dir/Ports.cpp.s"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/Ports.cpp -o CMakeFiles/pcn-lbrp.dir/Ports.cpp.s

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Service.cpp.o: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/flags.make
src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Service.cpp.o: ../src/services/pcn-loadbalancer-rp/src/Service.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_11) "Building CXX object src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Service.cpp.o"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/pcn-lbrp.dir/Service.cpp.o -c /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/Service.cpp

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Service.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/pcn-lbrp.dir/Service.cpp.i"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/Service.cpp > CMakeFiles/pcn-lbrp.dir/Service.cpp.i

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Service.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/pcn-lbrp.dir/Service.cpp.s"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/Service.cpp -o CMakeFiles/pcn-lbrp.dir/Service.cpp.s

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/ServiceBackend.cpp.o: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/flags.make
src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/ServiceBackend.cpp.o: ../src/services/pcn-loadbalancer-rp/src/ServiceBackend.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_12) "Building CXX object src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/ServiceBackend.cpp.o"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/pcn-lbrp.dir/ServiceBackend.cpp.o -c /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/ServiceBackend.cpp

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/ServiceBackend.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/pcn-lbrp.dir/ServiceBackend.cpp.i"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/ServiceBackend.cpp > CMakeFiles/pcn-lbrp.dir/ServiceBackend.cpp.i

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/ServiceBackend.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/pcn-lbrp.dir/ServiceBackend.cpp.s"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/ServiceBackend.cpp -o CMakeFiles/pcn-lbrp.dir/ServiceBackend.cpp.s

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/SrcIpRewrite.cpp.o: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/flags.make
src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/SrcIpRewrite.cpp.o: ../src/services/pcn-loadbalancer-rp/src/SrcIpRewrite.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_13) "Building CXX object src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/SrcIpRewrite.cpp.o"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/pcn-lbrp.dir/SrcIpRewrite.cpp.o -c /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/SrcIpRewrite.cpp

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/SrcIpRewrite.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/pcn-lbrp.dir/SrcIpRewrite.cpp.i"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/SrcIpRewrite.cpp > CMakeFiles/pcn-lbrp.dir/SrcIpRewrite.cpp.i

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/SrcIpRewrite.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/pcn-lbrp.dir/SrcIpRewrite.cpp.s"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/SrcIpRewrite.cpp -o CMakeFiles/pcn-lbrp.dir/SrcIpRewrite.cpp.s

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Lbrp-lib.cpp.o: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/flags.make
src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Lbrp-lib.cpp.o: ../src/services/pcn-loadbalancer-rp/src/Lbrp-lib.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_14) "Building CXX object src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Lbrp-lib.cpp.o"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/pcn-lbrp.dir/Lbrp-lib.cpp.o -c /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/Lbrp-lib.cpp

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Lbrp-lib.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/pcn-lbrp.dir/Lbrp-lib.cpp.i"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/Lbrp-lib.cpp > CMakeFiles/pcn-lbrp.dir/Lbrp-lib.cpp.i

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Lbrp-lib.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/pcn-lbrp.dir/Lbrp-lib.cpp.s"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pino/polycube/src/services/pcn-loadbalancer-rp/src/Lbrp-lib.cpp -o CMakeFiles/pcn-lbrp.dir/Lbrp-lib.cpp.s

# Object files for target pcn-lbrp
pcn__lbrp_OBJECTS = \
"CMakeFiles/pcn-lbrp.dir/serializer/JsonObjectBase.cpp.o" \
"CMakeFiles/pcn-lbrp.dir/serializer/LbrpJsonObject.cpp.o" \
"CMakeFiles/pcn-lbrp.dir/serializer/PortsJsonObject.cpp.o" \
"CMakeFiles/pcn-lbrp.dir/serializer/ServiceBackendJsonObject.cpp.o" \
"CMakeFiles/pcn-lbrp.dir/serializer/ServiceJsonObject.cpp.o" \
"CMakeFiles/pcn-lbrp.dir/serializer/SrcIpRewriteJsonObject.cpp.o" \
"CMakeFiles/pcn-lbrp.dir/api/LbrpApi.cpp.o" \
"CMakeFiles/pcn-lbrp.dir/api/LbrpApiImpl.cpp.o" \
"CMakeFiles/pcn-lbrp.dir/Lbrp.cpp.o" \
"CMakeFiles/pcn-lbrp.dir/Ports.cpp.o" \
"CMakeFiles/pcn-lbrp.dir/Service.cpp.o" \
"CMakeFiles/pcn-lbrp.dir/ServiceBackend.cpp.o" \
"CMakeFiles/pcn-lbrp.dir/SrcIpRewrite.cpp.o" \
"CMakeFiles/pcn-lbrp.dir/Lbrp-lib.cpp.o"

# External object files for target pcn-lbrp
pcn__lbrp_EXTERNAL_OBJECTS =

src/services/pcn-loadbalancer-rp/src/libpcn-lbrp.so: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/JsonObjectBase.cpp.o
src/services/pcn-loadbalancer-rp/src/libpcn-lbrp.so: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/LbrpJsonObject.cpp.o
src/services/pcn-loadbalancer-rp/src/libpcn-lbrp.so: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/PortsJsonObject.cpp.o
src/services/pcn-loadbalancer-rp/src/libpcn-lbrp.so: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/ServiceBackendJsonObject.cpp.o
src/services/pcn-loadbalancer-rp/src/libpcn-lbrp.so: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/ServiceJsonObject.cpp.o
src/services/pcn-loadbalancer-rp/src/libpcn-lbrp.so: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/serializer/SrcIpRewriteJsonObject.cpp.o
src/services/pcn-loadbalancer-rp/src/libpcn-lbrp.so: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/api/LbrpApi.cpp.o
src/services/pcn-loadbalancer-rp/src/libpcn-lbrp.so: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/api/LbrpApiImpl.cpp.o
src/services/pcn-loadbalancer-rp/src/libpcn-lbrp.so: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Lbrp.cpp.o
src/services/pcn-loadbalancer-rp/src/libpcn-lbrp.so: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Ports.cpp.o
src/services/pcn-loadbalancer-rp/src/libpcn-lbrp.so: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Service.cpp.o
src/services/pcn-loadbalancer-rp/src/libpcn-lbrp.so: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/ServiceBackend.cpp.o
src/services/pcn-loadbalancer-rp/src/libpcn-lbrp.so: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/SrcIpRewrite.cpp.o
src/services/pcn-loadbalancer-rp/src/libpcn-lbrp.so: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/Lbrp-lib.cpp.o
src/services/pcn-loadbalancer-rp/src/libpcn-lbrp.so: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/build.make
src/services/pcn-loadbalancer-rp/src/libpcn-lbrp.so: src/libs/polycube/src/libpolycube.a
src/services/pcn-loadbalancer-rp/src/libpcn-lbrp.so: src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_15) "Linking CXX shared library libpcn-lbrp.so"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/pcn-lbrp.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/build: src/services/pcn-loadbalancer-rp/src/libpcn-lbrp.so

.PHONY : src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/build

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/clean:
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src && $(CMAKE_COMMAND) -P CMakeFiles/pcn-lbrp.dir/cmake_clean.cmake
.PHONY : src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/clean

src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/depend:
	cd /home/pino/polycube/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/pino/polycube /home/pino/polycube/src/services/pcn-loadbalancer-rp/src /home/pino/polycube/cmake-build-debug /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src /home/pino/polycube/cmake-build-debug/src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/services/pcn-loadbalancer-rp/src/CMakeFiles/pcn-lbrp.dir/depend

