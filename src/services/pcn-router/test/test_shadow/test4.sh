#! /bin/bash
# 			  TOPOLOGY
#
#          +-----------+
#          | namespace |
#          +-----------+
#                ||
# Linux          ||
#________________||__________________
#                ||
# Polycube       ||
#             +------+
#             |  r1  |
#             +------+
#

source "${BASH_SOURCE%/*}/../helpers.bash"

function cleanup {
  set +e
  del_routers 1
}
trap cleanup EXIT

set -x
set -e

# Create the router r1 (shadow) and the port p1
polycubectl router add r1 shadow=true
router_add_port r1 p1 10.0.0.1 255.255.255.0

# Show router r1
polycubectl router r1 show

# Change the Ip address of the p1 port from Linux
sudo ip netns exec pcn-r1 ifconfig p1 192.168.0.1/24
polycubectl router r1 show

# Add route from Linux
sudo ip netns exec pcn-r1 ip route add 30.0.0.0/24 dev p1
polycubectl router r1 show

# Remove route from Linux
sudo ip netns exec pcn-r1 ip route del 30.0.0.0/24 dev p1
polycubectl router r1 show
