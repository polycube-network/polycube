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

# Utility rule file for generate_datapaths-Iptables_ChainForwarder_dp.h.

# Include the progress variables for this target.
include src/services/pcn-iptables/src/CMakeFiles/generate_datapaths-Iptables_ChainForwarder_dp.h.dir/progress.make

src/services/pcn-iptables/src/CMakeFiles/generate_datapaths-Iptables_ChainForwarder_dp.h: src/services/pcn-iptables/src/datapaths/Iptables_ChainForwarder_dp.h


src/services/pcn-iptables/src/datapaths/Iptables_ChainForwarder_dp.h: ../src/services/pcn-iptables/src/datapaths/Iptables_ChainForwarder_dp.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating datapaths/Iptables_ChainForwarder_dp.h"
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-iptables/src && mkdir -p /home/pino/polycube/cmake-build-debug/src/services/pcn-iptables/src/datapaths
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-iptables/src && echo "#pragma once" > /home/pino/polycube/cmake-build-debug/src/services/pcn-iptables/src/datapaths/Iptables_ChainForwarder_dp.h
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-iptables/src && echo "#include <string>" >> /home/pino/polycube/cmake-build-debug/src/services/pcn-iptables/src/datapaths/Iptables_ChainForwarder_dp.h
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-iptables/src && echo "const std::string iptables_code_chainforwarder = R\"POLYCUBE_DP(" >> /home/pino/polycube/cmake-build-debug/src/services/pcn-iptables/src/datapaths/Iptables_ChainForwarder_dp.h
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-iptables/src && cat /home/pino/polycube/src/services/pcn-iptables/src/datapaths/Iptables_ChainForwarder_dp.c >> /home/pino/polycube/cmake-build-debug/src/services/pcn-iptables/src/datapaths/Iptables_ChainForwarder_dp.h
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-iptables/src && cmake -E echo ")POLYCUBE_DP\";" >> /home/pino/polycube/cmake-build-debug/src/services/pcn-iptables/src/datapaths/Iptables_ChainForwarder_dp.h

generate_datapaths-Iptables_ChainForwarder_dp.h: src/services/pcn-iptables/src/CMakeFiles/generate_datapaths-Iptables_ChainForwarder_dp.h
generate_datapaths-Iptables_ChainForwarder_dp.h: src/services/pcn-iptables/src/datapaths/Iptables_ChainForwarder_dp.h
generate_datapaths-Iptables_ChainForwarder_dp.h: src/services/pcn-iptables/src/CMakeFiles/generate_datapaths-Iptables_ChainForwarder_dp.h.dir/build.make

.PHONY : generate_datapaths-Iptables_ChainForwarder_dp.h

# Rule to build all files generated by this target.
src/services/pcn-iptables/src/CMakeFiles/generate_datapaths-Iptables_ChainForwarder_dp.h.dir/build: generate_datapaths-Iptables_ChainForwarder_dp.h

.PHONY : src/services/pcn-iptables/src/CMakeFiles/generate_datapaths-Iptables_ChainForwarder_dp.h.dir/build

src/services/pcn-iptables/src/CMakeFiles/generate_datapaths-Iptables_ChainForwarder_dp.h.dir/clean:
	cd /home/pino/polycube/cmake-build-debug/src/services/pcn-iptables/src && $(CMAKE_COMMAND) -P CMakeFiles/generate_datapaths-Iptables_ChainForwarder_dp.h.dir/cmake_clean.cmake
.PHONY : src/services/pcn-iptables/src/CMakeFiles/generate_datapaths-Iptables_ChainForwarder_dp.h.dir/clean

src/services/pcn-iptables/src/CMakeFiles/generate_datapaths-Iptables_ChainForwarder_dp.h.dir/depend:
	cd /home/pino/polycube/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/pino/polycube /home/pino/polycube/src/services/pcn-iptables/src /home/pino/polycube/cmake-build-debug /home/pino/polycube/cmake-build-debug/src/services/pcn-iptables/src /home/pino/polycube/cmake-build-debug/src/services/pcn-iptables/src/CMakeFiles/generate_datapaths-Iptables_ChainForwarder_dp.h.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/services/pcn-iptables/src/CMakeFiles/generate_datapaths-Iptables_ChainForwarder_dp.h.dir/depend

