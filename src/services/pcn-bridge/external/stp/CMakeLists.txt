include_directories(${CMAKE_SOURCE_DIR}/external/stp)
add_library(stp STATIC stp.c dependencies.c log.cpp)
target_link_libraries(stp pthread)
set_property(TARGET stp PROPERTY POSITION_INDEPENDENT_CODE ON)
