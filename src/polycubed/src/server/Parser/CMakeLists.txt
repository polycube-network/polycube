find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

set(includes
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        )

set(parser_includes ${includes} PARENT_SCOPE)

bison_target(XPathParser
        XPath.yy
        ${CMAKE_CURRENT_BINARY_DIR}/XPathParser.cpp
        DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/XPathParser.h)

flex_target(XPathScanner
        XPath.flex
        ${CMAKE_CURRENT_BINARY_DIR}/XPathScanner.cpp
        COMPILE_FLAGS "-Cm -B -L --c++ --nounistd")

add_flex_bison_dependency(XPathScanner XPathParser)

set_source_files_properties(${BISON_XPathParser_OUTPUTS}
                            ${FLEX_XPathScanner_OUTPUTS}
                            PROPERTIES
                            COMPILE_FLAGS -Wno-sign-compare
                            COMPILE_FLAGS -Wno-effc++)

add_library(xpath STATIC
        ${CMAKE_CURRENT_LIST_DIR}/XPathParserDriver.cpp
        ${BISON_XPathParser_OUTPUTS}
        ${FLEX_XPathScanner_OUTPUTS})

target_include_directories(xpath PUBLIC ${includes})

set(parser_sources ${CMAKE_CURRENT_LIST_DIR}/Yang.cpp
        PARENT_SCOPE)