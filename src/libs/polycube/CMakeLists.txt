add_subdirectory(src)

configure_file(libpolycube.pc.in ${CMAKE_CURRENT_BINARY_DIR}/libpolycube.pc @ONLY)

# In order to use the TableDesc class inside the BaseCube and let Polycube services
# to be compiled outside of the base polycube services location,
# we need these BCC header files to be present on the host filesystem.
# By copying these source files from the BCC submodule to /usr/include/polycube,
# we ensure to have them updated to the last BCC submodule version used in polycube.
file(COPY
            ../bcc/src/cc/table_desc.h
            ../bcc/src/cc/file_desc.h
            ../bcc/src/cc/bcc_exception.h
        DESTINATION
            ${CMAKE_CURRENT_SOURCE_DIR}/include/polycube/services)

install(DIRECTORY include/ DESTINATION /usr/include)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpolycube.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)