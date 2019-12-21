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
include src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/depend.make

# Include the progress variables for this target.
include src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/progress.make

# Include the compile flags for this target's objects.
include src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/flags.make

src/polycubed/src/server/Parser/XPathParser.cpp: ../src/polycubed/src/server/Parser/XPath.yy
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "[BISON][XPathParser] Building parser with bison 3.0.4"
	cd /home/pino/polycube/src/polycubed/src/server/Parser && /usr/bin/bison --defines=/home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser/XPathParser.h -o /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser/XPathParser.cpp XPath.yy

src/polycubed/src/server/Parser/XPathParser.h: src/polycubed/src/server/Parser/XPathParser.cpp
	@$(CMAKE_COMMAND) -E touch_nocreate src/polycubed/src/server/Parser/XPathParser.h

src/polycubed/src/server/Parser/XPathScanner.cpp: ../src/polycubed/src/server/Parser/XPath.flex
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "[FLEX][XPathScanner] Building scanner with flex 2.6.4"
	cd /home/pino/polycube/src/polycubed/src/server/Parser && /usr/bin/flex -Cm -B -L --c++ --nounistd -o/home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser/XPathScanner.cpp XPath.flex

src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/XPathParserDriver.cpp.o: src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/flags.make
src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/XPathParserDriver.cpp.o: ../src/polycubed/src/server/Parser/XPathParserDriver.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/XPathParserDriver.cpp.o"
	cd /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/xpath.dir/XPathParserDriver.cpp.o -c /home/pino/polycube/src/polycubed/src/server/Parser/XPathParserDriver.cpp

src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/XPathParserDriver.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/xpath.dir/XPathParserDriver.cpp.i"
	cd /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pino/polycube/src/polycubed/src/server/Parser/XPathParserDriver.cpp > CMakeFiles/xpath.dir/XPathParserDriver.cpp.i

src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/XPathParserDriver.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/xpath.dir/XPathParserDriver.cpp.s"
	cd /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pino/polycube/src/polycubed/src/server/Parser/XPathParserDriver.cpp -o CMakeFiles/xpath.dir/XPathParserDriver.cpp.s

src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/XPathParser.cpp.o: src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/flags.make
src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/XPathParser.cpp.o: src/polycubed/src/server/Parser/XPathParser.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/XPathParser.cpp.o"
	cd /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -Wno-effc++ -o CMakeFiles/xpath.dir/XPathParser.cpp.o -c /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser/XPathParser.cpp

src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/XPathParser.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/xpath.dir/XPathParser.cpp.i"
	cd /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -Wno-effc++ -E /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser/XPathParser.cpp > CMakeFiles/xpath.dir/XPathParser.cpp.i

src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/XPathParser.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/xpath.dir/XPathParser.cpp.s"
	cd /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -Wno-effc++ -S /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser/XPathParser.cpp -o CMakeFiles/xpath.dir/XPathParser.cpp.s

src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/XPathScanner.cpp.o: src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/flags.make
src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/XPathScanner.cpp.o: src/polycubed/src/server/Parser/XPathScanner.cpp
src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/XPathScanner.cpp.o: src/polycubed/src/server/Parser/XPathParser.h
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/XPathScanner.cpp.o"
	cd /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -Wno-effc++ -o CMakeFiles/xpath.dir/XPathScanner.cpp.o -c /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser/XPathScanner.cpp

src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/XPathScanner.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/xpath.dir/XPathScanner.cpp.i"
	cd /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -Wno-effc++ -E /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser/XPathScanner.cpp > CMakeFiles/xpath.dir/XPathScanner.cpp.i

src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/XPathScanner.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/xpath.dir/XPathScanner.cpp.s"
	cd /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -Wno-effc++ -S /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser/XPathScanner.cpp -o CMakeFiles/xpath.dir/XPathScanner.cpp.s

# Object files for target xpath
xpath_OBJECTS = \
"CMakeFiles/xpath.dir/XPathParserDriver.cpp.o" \
"CMakeFiles/xpath.dir/XPathParser.cpp.o" \
"CMakeFiles/xpath.dir/XPathScanner.cpp.o"

# External object files for target xpath
xpath_EXTERNAL_OBJECTS =

src/polycubed/src/server/Parser/libxpath.a: src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/XPathParserDriver.cpp.o
src/polycubed/src/server/Parser/libxpath.a: src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/XPathParser.cpp.o
src/polycubed/src/server/Parser/libxpath.a: src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/XPathScanner.cpp.o
src/polycubed/src/server/Parser/libxpath.a: src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/build.make
src/polycubed/src/server/Parser/libxpath.a: src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Linking CXX static library libxpath.a"
	cd /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser && $(CMAKE_COMMAND) -P CMakeFiles/xpath.dir/cmake_clean_target.cmake
	cd /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/xpath.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/build: src/polycubed/src/server/Parser/libxpath.a

.PHONY : src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/build

src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/clean:
	cd /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser && $(CMAKE_COMMAND) -P CMakeFiles/xpath.dir/cmake_clean.cmake
.PHONY : src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/clean

src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/depend: src/polycubed/src/server/Parser/XPathParser.cpp
src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/depend: src/polycubed/src/server/Parser/XPathParser.h
src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/depend: src/polycubed/src/server/Parser/XPathScanner.cpp
	cd /home/pino/polycube/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/pino/polycube /home/pino/polycube/src/polycubed/src/server/Parser /home/pino/polycube/cmake-build-debug /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser /home/pino/polycube/cmake-build-debug/src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/polycubed/src/server/Parser/CMakeFiles/xpath.dir/depend

