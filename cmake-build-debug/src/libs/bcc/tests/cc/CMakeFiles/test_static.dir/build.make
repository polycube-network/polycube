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
include src/libs/bcc/tests/cc/CMakeFiles/test_static.dir/depend.make

# Include the progress variables for this target.
include src/libs/bcc/tests/cc/CMakeFiles/test_static.dir/progress.make

# Include the compile flags for this target's objects.
include src/libs/bcc/tests/cc/CMakeFiles/test_static.dir/flags.make

src/libs/bcc/tests/cc/CMakeFiles/test_static.dir/test_static.c.o: src/libs/bcc/tests/cc/CMakeFiles/test_static.dir/flags.make
src/libs/bcc/tests/cc/CMakeFiles/test_static.dir/test_static.c.o: ../src/libs/bcc/tests/cc/test_static.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object src/libs/bcc/tests/cc/CMakeFiles/test_static.dir/test_static.c.o"
	cd /home/pino/polycube/cmake-build-debug/src/libs/bcc/tests/cc && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/test_static.dir/test_static.c.o   -c /home/pino/polycube/src/libs/bcc/tests/cc/test_static.c

src/libs/bcc/tests/cc/CMakeFiles/test_static.dir/test_static.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/test_static.dir/test_static.c.i"
	cd /home/pino/polycube/cmake-build-debug/src/libs/bcc/tests/cc && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pino/polycube/src/libs/bcc/tests/cc/test_static.c > CMakeFiles/test_static.dir/test_static.c.i

src/libs/bcc/tests/cc/CMakeFiles/test_static.dir/test_static.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/test_static.dir/test_static.c.s"
	cd /home/pino/polycube/cmake-build-debug/src/libs/bcc/tests/cc && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pino/polycube/src/libs/bcc/tests/cc/test_static.c -o CMakeFiles/test_static.dir/test_static.c.s

# Object files for target test_static
test_static_OBJECTS = \
"CMakeFiles/test_static.dir/test_static.c.o"

# External object files for target test_static
test_static_EXTERNAL_OBJECTS =

src/libs/bcc/tests/cc/test_static: src/libs/bcc/tests/cc/CMakeFiles/test_static.dir/test_static.c.o
src/libs/bcc/tests/cc/test_static: src/libs/bcc/tests/cc/CMakeFiles/test_static.dir/build.make
src/libs/bcc/tests/cc/test_static: src/libs/bcc/src/cc/libbcc.a
src/libs/bcc/tests/cc/test_static: src/libs/bcc/src/cc/frontends/b/libb_frontend.a
src/libs/bcc/tests/cc/test_static: src/libs/bcc/src/cc/frontends/clang/libclang_frontend.a
src/libs/bcc/tests/cc/test_static: src/libs/bcc/src/cc/libbcc_bpf.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libclangFrontend.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libclangSerialization.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libclangDriver.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libclangParse.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libclangSema.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libclangCodeGen.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libclangAnalysis.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libclangRewrite.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libclangEdit.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libclangAST.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libclangLex.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libclangBasic.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMCoroutines.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMCoverage.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMX86CodeGen.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMX86Desc.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMX86Info.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMMCDisassembler.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMX86AsmPrinter.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMX86Utils.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMGlobalISel.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMLTO.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMPasses.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMipo.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMVectorize.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMInstrumentation.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMOption.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMObjCARCOpts.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMMCJIT.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMExecutionEngine.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMRuntimeDyld.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMLinker.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMIRReader.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMAsmParser.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMDebugInfoDWARF.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMBPFCodeGen.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMSelectionDAG.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMBPFDesc.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMBPFInfo.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMBPFAsmPrinter.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMAsmPrinter.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMDebugInfoCodeView.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMDebugInfoMSF.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMCodeGen.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMTarget.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMScalarOpts.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMInstCombine.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMTransformUtils.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMBitWriter.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMAnalysis.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMProfileData.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMObject.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMMCParser.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMMC.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMBitReader.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMCore.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMBinaryFormat.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMSupport.a
src/libs/bcc/tests/cc/test_static: /usr/lib/llvm-5.0/lib/libLLVMDemangle.a
src/libs/bcc/tests/cc/test_static: /usr/lib/x86_64-linux-gnu/libelf.so
src/libs/bcc/tests/cc/test_static: src/libs/bcc/src/cc/api/libapi-static.a
src/libs/bcc/tests/cc/test_static: src/libs/bcc/src/cc/usdt/libusdt-static.a
src/libs/bcc/tests/cc/test_static: src/libs/bcc/src/cc/libbcc-loader-static.a
src/libs/bcc/tests/cc/test_static: src/libs/bcc/tests/cc/CMakeFiles/test_static.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/pino/polycube/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable test_static"
	cd /home/pino/polycube/cmake-build-debug/src/libs/bcc/tests/cc && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_static.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/libs/bcc/tests/cc/CMakeFiles/test_static.dir/build: src/libs/bcc/tests/cc/test_static

.PHONY : src/libs/bcc/tests/cc/CMakeFiles/test_static.dir/build

src/libs/bcc/tests/cc/CMakeFiles/test_static.dir/clean:
	cd /home/pino/polycube/cmake-build-debug/src/libs/bcc/tests/cc && $(CMAKE_COMMAND) -P CMakeFiles/test_static.dir/cmake_clean.cmake
.PHONY : src/libs/bcc/tests/cc/CMakeFiles/test_static.dir/clean

src/libs/bcc/tests/cc/CMakeFiles/test_static.dir/depend:
	cd /home/pino/polycube/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/pino/polycube /home/pino/polycube/src/libs/bcc/tests/cc /home/pino/polycube/cmake-build-debug /home/pino/polycube/cmake-build-debug/src/libs/bcc/tests/cc /home/pino/polycube/cmake-build-debug/src/libs/bcc/tests/cc/CMakeFiles/test_static.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/libs/bcc/tests/cc/CMakeFiles/test_static.dir/depend

