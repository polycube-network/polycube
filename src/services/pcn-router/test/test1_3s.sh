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

# Create the router r1
set -e
add_routers 1

# Creates three ports on the router, each one attached to a different namespace
router_add_port r1 to_veth1 10.0.0.120 255.255.255.0
router_add_port r1 to_veth2 10.10.20.10 255.255.255.0
router_add_port r1 to_veth3 10.30.10.10 255.255.255.0

# Configure secondary addresses on port veth1
router_add_secondary r1 veth1 10.10.10.1 255.255.255.0
router_add_secondary r1 veth1 10.10.21.2 255.255.255.0
router_add_secondary_as_gateway r1 veth1 1

# Configure secondary addresses on port veth2
router_add_secondary r1 veth2 10.14.1.1 255.255.255.0
router_add_secondary r1 veth2 10.1.15.2 255.255.255.0
router_add_secondary_as_gateway r1 veth2 2

# Configure secondary addresses on port veth3
router_add_secondary r1 veth3 11.1.1.1 255.255.255.0
router_add_secondary r1 veth3 12.1.1.1 255.255.255.0
router_add_secondary_as_gateway r1 veth3 3

#ns1 on port veth1
sudo ip netns exec ns1 ping 10.0.0.120 -c 2 -i 0.5
sudo ip netns exec ns1 ping 10.10.20.10 -c 2 -i 0.5
sudo ip netns exec ns1 ping 10.30.10.10 -c 2 -i 0.5
sudo ip netns exec ns1 ping 10.10.10.1 -c 2 -i 0.5
sudo ip netns exec ns1 ping 10.10.21.2 -c 2 -i 0.5
sudo ip netns exec ns1 ping 10.0.1.254 -c 2 -i 0.5
#ns1 on port veth2
sudo ip netns exec ns1 ping 10.14.1.1 -c 2 -i 0.5
sudo ip netns exec ns1 ping 10.1.15.2 -c 2 -i 0.5
sudo ip netns exec ns1 ping 10.0.2.254 -c 2 -i 0.5
#ns3 on port veth3
sudo ip netns exec ns1 ping 11.1.1.1 -c 2 -i 0.5
sudo ip netns exec ns1 ping 12.1.1.1 -c 2 -i 0.5
sudo ip netns exec ns1 ping 10.0.3.254 -c 2 -i 0.5

#ns2 on port veth1
sudo ip netns exec ns2 ping 10.0.0.120 -c 2 -i 0.5
sudo ip netns exec ns2 ping 10.10.20.10 -c 2 -i 0.5
sudo ip netns exec ns2 ping 10.30.10.10 -c 2 -i 0.5
sudo ip netns exec ns2 ping 10.10.10.1 -c 2 -i 0.5
sudo ip netns exec ns2 ping 10.10.21.2 -c 2 -i 0.5
sudo ip netns exec ns2 ping 10.0.1.254 -c 2 -i 0.5
#ns2 on port veth2
sudo ip netns exec ns2 ping 10.14.1.1 -c 2 -i 0.5
sudo ip netns exec ns2 ping 10.1.15.2 -c 2 -i 0.5
sudo ip netns exec ns2 ping 10.0.2.254 -c 2 -i 0.5
#ns2 on port veth3
sudo ip netns exec ns2 ping 11.1.1.1 -c 2 -i 0.5
sudo ip netns exec ns2 ping 12.1.1.1 -c 2 -i 0.5
sudo ip netns exec ns2 ping 10.0.3.254 -c 2 -i 0.5

#ns3 on port veth1
sudo ip netns exec ns3 ping 10.0.0.120 -c 2 -i 0.5
sudo ip netns exec ns3 ping 10.10.20.10 -c 2 -i 0.5
sudo ip netns exec ns3 ping 10.30.10.10 -c 2 -i 0.5
sudo ip netns exec ns3 ping 10.10.10.1 -c 2 -i 0.5
sudo ip netns exec ns3 ping 10.10.21.2 -c 2 -i 0.5
sudo ip netns exec ns3 ping 10.0.1.254 -c 2 -i 0.5
#ns3 on port veth2
sudo ip netns exec ns3 ping 10.14.1.1 -c 2 -i 0.5
sudo ip netns exec ns3 ping 10.1.15.2 -c 2 -i 0.5
sudo ip netns exec ns3 ping 10.0.2.254 -c 2 -i 0.5
#ns3 on port veth3
sudo ip netns exec ns3 ping 11.1.1.1 -c 2 -i 0.5
sudo ip netns exec ns3 ping 12.1.1.1 -c 2 -i 0.5
sudo ip netns exec ns3 ping 10.0.3.254 -c 2 -i 0.5

ping_cycle 3
