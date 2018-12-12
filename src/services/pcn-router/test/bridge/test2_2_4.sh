#! /bin/bash

#                        TOPOLOGY
#
#
#            veth1                                           veth3
#              |                                               |
#  (Network)   |                                               |    (Network)
# 10.0.1.0/24  |                                               |   10.0.2.0/24
#              |                                               |
#          +-------+      +------+           +------+      +-------+
#          |  br1  |------|  r1  |-----------|  r2  |------|  br2  |
#          +-------+      +------+           +------+      +-------+
#              |                   (Network)                   |
#              |                  10.1.0.0/30                  |
#              |                                               |
#            veth2                                           veth4
#
#

source "${BASH_SOURCE%/*}/../helpers.bash"
source "${BASH_SOURCE%/*}/helpers_bridge.bash"
function cleanup {
  set +e
  del_bridges 2
  del_routers 2
  delete_veth 4
}
trap cleanup EXIT

set -x
create_veth_subnet 4 2

set -e

TYPE="TC"

if [ -n "$1" ]; then
  TYPE=$1
fi

polycubectl router add r1 type=$TYPE
polycubectl router add r2 type=$TYPE
polycubectl simplebridge add br1 type=$TYPE
polycubectl simplebridge add br2 type=$TYPE

router_add_port r1 to_br1 10.10.2.1 255.255.255.0

bridge_add_port br1 veth1
bridge_add_port br1 veth2

router_add_secondary_as_gateway r1 br1 1 br1
connect_router_p_to_p r1 r2 1

bridge_add_port br2 veth3
bridge_add_port br2 veth4

router_add_port_as_gateway r2 br2 2 br2

router_add_route r1 10.0.2.0 255.255.255.0 10.1.0.2
router_add_route r2 10.0.1.0 255.255.255.0 10.1.0.1
router_add_route r2 10.10.2.1 255.255.255.255 10.1.0.1

#sleep 40s #for fill filtering database

ping_cycle_subnet 4 2

ping_special 4 10.10.2.1 10.1.0.1 10.1.0.2
