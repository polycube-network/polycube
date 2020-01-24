include_directories(${CMAKE_SOURCE_DIR}/src/libs/polycube/include)
include_directories(${CMAKE_SOURCE_DIR}/src/libs/spdlog)

set(POLYCUBE_STANDALONE_SERVICE false)
set(POLYCUBE_LIBRARIES polycube uuid tins)

set(LOAD_SERVICES "" PARENT_SCOPE)

macro(add_service servicename servicefolder)
    string(TOUPPER ${servicename} servicenameupper)
    option(ENABLE_SERVICE_${servicenameupper} "compiles the ${servicename} service" ON)
    if (ENABLE_SERVICE_${servicenameupper})
        add_subdirectory(${servicefolder})
        string(CONCAT LOAD_SERVICES "${LOAD_SERVICES}" "try_to_load(\"${servicename}\", \"libpcn-${servicename}.so\");\n")
    endif (ENABLE_SERVICE_${servicenameupper})
endmacro()

# put your new service here
# first argument is the service name used in the rest API
# second argument is the folder of the service

add_service(bridge pcn-bridge)
add_service(ddosmitigator pcn-ddosmitigator)
add_service(firewall pcn-firewall)
add_service(helloworld pcn-helloworld)
add_service(k8switch pcn-k8switch)
add_service(k8sfilter pcn-k8sfilter)
add_service(lbdsr pcn-loadbalancer-dsr)
add_service(lbrp pcn-loadbalancer-rp)
add_service(nat pcn-nat)
add_service(pbforwarder pcn-pbforwarder)
add_service(router pcn-router)
add_service(simplebridge pcn-simplebridge)
add_service(simpleforwarder pcn-simpleforwarder)
add_service(iptables pcn-iptables)
add_service(transparenthelloworld pcn-transparent-helloworld)
add_service(synflood pcn-synflood)
add_service(packetcapture pcn-packetcapture)
add_service(dynmon pcn-dynmon)
add_service(policer pcn-policer)

# save string to create code that load the services
SET_PROPERTY(GLOBAL PROPERTY LOAD_SERVICES_ ${LOAD_SERVICES})

install(DIRECTORY datamodel-common DESTINATION include/polycube)
