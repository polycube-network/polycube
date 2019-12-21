# CMake generated Testfile for 
# Source directory: /home/pino/polycube/src/libs/bcc/tests/cc
# Build directory: /home/pino/polycube/cmake-build-debug/src/libs/bcc/tests/cc
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(c_test_static "/home/pino/polycube/cmake-build-debug/src/libs/bcc/tests/wrapper.sh" "c_test_static" "sudo" "/home/pino/polycube/cmake-build-debug/src/libs/bcc/tests/cc/test_static")
set_tests_properties(c_test_static PROPERTIES  _BACKTRACE_TRIPLES "/home/pino/polycube/src/libs/bcc/tests/cc/CMakeLists.txt;11;add_test;/home/pino/polycube/src/libs/bcc/tests/cc/CMakeLists.txt;0;")
add_test(test_libbcc "/home/pino/polycube/cmake-build-debug/src/libs/bcc/tests/wrapper.sh" "c_test_all" "sudo" "/home/pino/polycube/cmake-build-debug/src/libs/bcc/tests/cc/test_libbcc")
set_tests_properties(test_libbcc PROPERTIES  _BACKTRACE_TRIPLES "/home/pino/polycube/src/libs/bcc/tests/cc/CMakeLists.txt;31;add_test;/home/pino/polycube/src/libs/bcc/tests/cc/CMakeLists.txt;0;")
