cmake_minimum_required (VERSION 3.2)

set (CMAKE_CXX_STANDARD 11)

include_directories(external)
add_subdirectory(src)
add_subdirectory(external/stp)
