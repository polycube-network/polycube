#! /bin/bash

#                        TOPOLOGY
#
#
#            veth1                                           veth3
#              |                                               |
#  (Network)   |                                               |    (Network)
# 10.0.1.0/24  |                                               |   10.0.3.0/24
#              |                                               |
#          +-------+      +------+           +------+      +-------+
#          |  br1  |------|  r1  |-----------|  r2  |------|  br2  |---------veth5  (Network)
#          +-------+      +------+           +------+      +-------+                10.0.5.0/24
#              |                   (Network)                   |
#  (Network)   |                  10.1.0.0/30                  |     (Network)
# 10.0.2.0/24  |                                               |    10.0.4.0/24
#            veth2                                           veth4
#
#

source "${BASH_SOURCE%/*}/../helpers.bash"
source "${BASH_SOURCE%/*}/helpers_bridge.bash"

function cleanup {
  set +e
  del_bridges 2
  del_routers 2
  delete_veth 5
}
trap cleanup EXIT

set -x
create_veth_net 5

set -e

add_routers 2
add_bridges 2

router_add_port_as_gateway r1 br1 1 br1
router_add_secondary r1 br1 10.0.2.254 255.255.255.0 #DG for veth2

connect_router_p_to_p r1 r2 1
router_add_secondary r1 r2 10.10.0.1 255.255.255.252
router_add_secondary r2 r1 10.10.0.2 255.255.255.252   #secondary nexthop

bridge_add_port br1 veth1
bridge_add_port br1 veth2

router_add_port_as_gateway r2 br2 3 br2
router_add_secondary r2 br2 10.0.4.254 255.255.255.0 #DG for veth4
router_add_secondary r2 br2 10.0.5.254 255.255.255.0 #DG for veth5

bridge_add_port br2 veth3
bridge_add_port br2 veth4
bridge_add_port br2 veth5

router_add_route r1 10.0.3.0 255.255.255.0 10.1.0.2 5
router_add_route r1 10.0.4.0 255.255.255.0 10.1.0.2 5
router_add_route r1 10.0.5.0 255.255.255.0 10.1.0.2 5

router_add_route r2 10.0.1.0 255.255.255.0 10.1.0.1 5
router_add_route r2 10.0.2.0 255.255.255.0 10.1.0.1 5

#minor patchost
router_add_route r1 10.0.3.0 255.255.255.0 10.10.0.2 2

ping_cycle 5
ping_special 5 10.10.0.1 10.10.0.2

polycubectl router r1 ports to_r2 secondaryip del 10.10.0.1 255.255.255.252

ping_cycle 5

#The following command should fail, since r1 does no longer have routes to the network 10.10.0.0/30
ping_special_fail 5 10.10.0.1
