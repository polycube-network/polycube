#!/bin/bash

source "${BASH_SOURCE%/*}/../helpers.bash"

function cleanup {
  set +e
  del_routers 1
}
trap cleanup EXIT

set -x
set -e

# Create the router r1
polycubectl router add r1 shadow=true
# Check if the namespace is created
sudo ip netns ls

# Add port p1
router_add_port r1 p1 10.0.0.1/24
sudo ip netns exec pcn-r1 ip -c a

# Change the Ip address of the p1 port from Linux
sudo ip netns exec pcn-r1 ifconfig p1 10.0.0.2/24
polycubectl router r1 ports p1 show

# Add route from Linux
sudo ip netns exec pcn-r1 ip route add 30.0.0.0/24 dev p1
polycubectl router r1 route show
