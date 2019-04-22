add_subdirectory(src)

configure_file(libpolycube.pc.in ${CMAKE_CURRENT_BINARY_DIR}/libpolycube.pc @ONLY)

install(DIRECTORY include/ DESTINATION /usr/include)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpolycube.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)