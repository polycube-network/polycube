#! /bin/bash

#                      TOPOLOGY
#
#                                     veth3
#                                       |
#                                       |
#                                       |
#                     +------+      +-------+
#            veth1    |  r2  |------|  br2  |
#              |      +------+      +-------+
#              |     /                  |
#              |    /                   |
#          +-------+                    |
#          |  br1  |                  veth4
#          +-------+
#              |    \
#              |     \
#              |      +------+
#            veth2    |  r1  | (DG for veth1 e veth2)
#                     +------+
#

source "${BASH_SOURCE%/*}/../helpers.bash"
source "${BASH_SOURCE%/*}/helpers_bridge.bash"

function cleanup {
  set +e
  del_routers 2
  del_bridges 2
  delete_veth 4
}
trap cleanup EXIT

set -x
create_veth_subnet 4 2

set -e

add_routers 2
add_bridges 2

router_add_port_as_gateway r1 br1 1 br1
bridge_add_port br1 veth1
bridge_add_port br1 veth2

polycubectl simplebridge br1 ports add to_r2
router_add_port_as_gateway r2 br2 2 br2
router_add_port r2 to_br1 10.0.1.253 255.255.255.0

bridge_add_port br2 veth3
bridge_add_port br2 veth4

polycubectl connect br1:to_r2 r2:to_br1

router_add_route r1 10.0.2.0 255.255.255.0 10.0.1.253

router_add_route r2 10.0.1.0 255.255.255.0 10.0.1.254 #ignored, local entry is preferred

sudo ip netns exec ns1 ping 10.0.1.2 -c 2 -i 0.5
sudo ip netns exec ns1 ping 10.0.2.1 -c 2 -i 0.5
sudo ip netns exec ns1 ping 10.0.2.2 -c 2 -i 0.5

sudo ip netns exec ns2 ping 10.0.1.1 -c 2 -i 0.5
sudo ip netns exec ns2 ping 10.0.2.1 -c 2 -i 0.5
sudo ip netns exec ns2 ping 10.0.2.2 -c 2 -i 0.5

sudo ip netns exec ns3 ping 10.0.1.1 -c 2 -i 0.5
sudo ip netns exec ns3 ping 10.0.1.2 -c 2 -i 0.5
sudo ip netns exec ns3 ping 10.0.2.2 -c 2 -i 0.5

sudo ip netns exec ns4 ping 10.0.1.1 -c 2 -i 0.5
sudo ip netns exec ns4 ping 10.0.1.2 -c 2 -i 0.5
sudo ip netns exec ns4 ping 10.0.2.1 -c 2 -i 0.5
