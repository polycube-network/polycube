#! /bin/bash
# 			  TOPOLOGY
#
#
#
#                         +------+
#             veth1 ------|  r1  |------- veth2
#                         +------+
#                             |
#                             |
#                             |
#                           veth3


source "${BASH_SOURCE%/*}/helpers.bash"

function cleanup {
  set +e
  del_routers 1
  delete_veth 3
}
trap cleanup EXIT

TYPE="TC"

if [ -n "$1" ]; then
  TYPE=$1
fi

# Create 3 namespaces
# - "ns1", attached to the interface veth1. Such a network interface has
#   ip address  10.0.1.1/24, while the address  10.0.1.254 is set as a default gateway
# - "ns2", attached to the interface veth2. Such a network interface has
#   ip address  10.0.2.1/24, while the address  10.0.2.254 is set as a default gateway
# - "ns3", attached to the interface veth1. Such a network interface has
#   ip address  10.0.3.1/24, while the address  10.0.3.254 is set as a default gateway
set -x
create_veth_net 3

# You can use the following commands to check that everything has been set up correctly

# sudo ip netns ls
# sudo ip netns exec ns3 [command]

set -e

# Create the router r1
polycubectl router add r1 type=$TYPE

# Attaches the three ports to the router
router_add_port_as_gateway r1 veth1 1
router_add_port_as_gateway r1 veth2 2
router_add_port_as_gateway r1 veth3 3

# All the namespaces try to ping each other
ping_cycle 3
