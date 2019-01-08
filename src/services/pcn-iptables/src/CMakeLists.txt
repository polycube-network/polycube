include(${PROJECT_SOURCE_DIR}/cmake/LoadFileAsVariable.cmake)

aux_source_directory(serializer SERIALIZER_SOURCES)
aux_source_directory(api API_SOURCES)
aux_source_directory(default-src SRC_SOURCES)
aux_source_directory(modules MODULES)

include_directories(serializer)
include_directories(interface)
include_directories(default-src)
include_directories(modules)
include_directories(/usr/include/libnl3)

# Needed to load files as variables
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(pcn-iptables SHARED
  ${SERIALIZER_SOURCES}
  ${API_SOURCES}
  ${SRC_SOURCES}
  ${MODULES}
  Chain.cpp
  ChainRule.cpp
  ChainStats.cpp
  Iptables.cpp
  Ports.cpp
  SessionTable.cpp
  Iptables-lib.cpp
  Utils.cpp)

# takes 'Router_dp.c' and make it available in 'Router_dp.h'
# as a variable called 'router_code'
load_file_as_variable(pcn-iptables datapaths/Iptables_ActionLookup_dp.c iptables_code_actionlookup)
load_file_as_variable(pcn-iptables datapaths/Iptables_BitScan_dp.c iptables_code_bitscan)
load_file_as_variable(pcn-iptables datapaths/Iptables_ChainForwarder_dp.c iptables_code_chainforwarder)
load_file_as_variable(pcn-iptables datapaths/Iptables_ChainSelector_dp.c iptables_code_chainselector)
load_file_as_variable(pcn-iptables datapaths/Iptables_ConntrackLabel_dp.c iptables_code_conntracklabel)
load_file_as_variable(pcn-iptables datapaths/Iptables_ConntrackMatch_dp.c iptables_code_conntrackmatch)
load_file_as_variable(pcn-iptables datapaths/Iptables_ConntrackTableUpdate_dp.c iptables_code_conntracktableupdate)
load_file_as_variable(pcn-iptables datapaths/Iptables_Horus_dp.c iptables_code_horus)
load_file_as_variable(pcn-iptables datapaths/Iptables_IpLookup_dp.c iptables_code_iplookup)
load_file_as_variable(pcn-iptables datapaths/Iptables_L4PortLookup_dp.c iptables_code_l4portlookup)
load_file_as_variable(pcn-iptables datapaths/Iptables_L4ProtocolLookup_dp.c iptables_code_l4protolookup)
load_file_as_variable(pcn-iptables datapaths/Iptables_Parser_dp.c iptables_code_parser)
load_file_as_variable(pcn-iptables datapaths/Iptables_TcpFlagsLookup_dp.c iptables_code_tcpflagslookup)

target_link_libraries(pcn-iptables
  polycube
  uuid)

# Specify shared library install directory

set(CMAKE_INSTALL_LIBDIR /usr/lib)

install(
    TARGETS
    pcn-iptables
    DESTINATION
    "${CMAKE_INSTALL_LIBDIR}"
)
