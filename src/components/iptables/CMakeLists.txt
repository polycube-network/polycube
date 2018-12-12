add_subdirectory(scripts)

set(IPTABLES_INSTALL_DIR /usr/local/share/polycube/pcn-iptables/)

if (ENABLE_PCN_IPTABLES)
  include(ExternalProject)
  ExternalProject_Add(iptables
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/iptables/
    CONFIGURE_COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/iptables/ && ./autogen.sh && ./configure --prefix=${IPTABLES_INSTALL_DIR} --disable-nftables
    BUILD_COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/iptables && make
    INSTALL_COMMAND exit
  )

  install(CODE "execute_process(WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/iptables COMMAND make install)")
endif(ENABLE_PCN_IPTABLES)
