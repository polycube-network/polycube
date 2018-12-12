#! /bin/bash

#               TOPOLOGY
#
#
#              +-------+          +-------+
#  veth1 ------|  r1   |----------|   r2  | ------ veth2
#              +-------+          +-------+
#                   \                 /
#                    \               /
#                     \             /
#                      \ +-------+ /
#                       \|   r3  |/
#                        +-------+
#                            |
#                            |
#                            |
#                          veth3

source "${BASH_SOURCE%/*}/helpers.bash"

function cleanup {
  set +e
  del_routers 3
  delete_veth 3
}
trap cleanup EXIT

set -x
create_veth_net 3

set -e

TYPE="TC"

if [ -n "$1" ]; then
  TYPE=$1
fi

polycubectl router add r1 type=$TYPE
polycubectl router add r2 type=$TYPE
polycubectl router add r3 type=$TYPE

router_add_port_as_gateway r1 veth1 1

router_add_port r1 to_r2 10.10.10.10 255.255.255.0
router_add_port r2 to_r1 10.10.20.10 255.255.255.0
connect_router_secondary_p_to_p r1 r2 1

router_add_port r2 to_veth2 12.10.1.1 255.255.255.0
router_add_secondary_as_gateway r2 veth2 2

connect_router_p_to_p r1 r3 2
connect_router_p_to_p r2 r3 3

router_add_port_as_gateway r3 veth3 3

#router_add_route r1 10.0.2.0 255.255.255.0 10.1.0.2
#router_add_route r1 10.0.3.0 255.255.255.0 10.2.0.2
router_add_route r1 0.0.0.0 0.0.0.0 10.1.0.2

router_add_route r2 10.0.1.0 255.255.255.0 10.1.0.1
router_add_route r2 10.0.3.0 255.255.255.0 10.3.0.2

router_add_route r3 10.0.1.0 255.255.255.0 10.2.0.1
router_add_route r3 10.0.2.0 255.255.255.0 10.3.0.1
router_add_route r3 10.10.10.10 255.255.255.255 10.2.0.1
router_add_route r3 10.10.20.10 255.255.255.255 10.3.0.1

router_routingtable_show r1
router_routingtable_show r2
router_routingtable_show r3

sudo ip netns exec ns3 ping 10.10.10.10 -c 2 -i 0.5 -t 10
sudo ip netns exec ns3 ping 10.10.20.10 -c 8 -t 10

ping_cycle 3

#test ICMP ttl exceeded packet
test_fail sudo ip netns exec ns1 ping 10.0.2.2 -b -t1 -c 2 -i 0.5


