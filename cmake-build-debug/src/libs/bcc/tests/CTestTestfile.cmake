# CMake generated Testfile for 
# Source directory: /home/pino/polycube/src/libs/bcc/tests
# Build directory: /home/pino/polycube/cmake-build-debug/src/libs/bcc/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(style-check "/home/pino/polycube/scripts/c-style-check.sh")
set_tests_properties(style-check PROPERTIES  PASS_REGULAR_EXPRESSION ".*" _BACKTRACE_TRIPLES "/home/pino/polycube/src/libs/bcc/tests/CMakeLists.txt;7;add_test;/home/pino/polycube/src/libs/bcc/tests/CMakeLists.txt;0;")
subdirs("cc")
subdirs("python")
subdirs("lua")
