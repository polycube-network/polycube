#! /bin/bash

# 4 pcn-bridge; square topology;
# connect extra links between bridges 3-4
# test connectivity, after convergence

source "${BASH_SOURCE%/*}/../../helpers.bash"

function cleanup {
  set +e
  del_bridges 2
  del_bridges_lb 3 4
  delete_veth 4
  delete_link 5
}
trap cleanup EXIT

set -x

#setup
create_veth 4
create_link 5

set -e

add_bridges_stp 2
add_bridges_lb 3 4

# create ports
bridge_add_port br1 link11
polycubectl bridge br1 stp 1 set priority=28672
bridge_add_port br1 link42
bridge_add_port br1 veth1

bridge_add_port br2 link12
polycubectl bridge br2 stp 1 set priority=24576
bridge_add_port br2 link21
bridge_add_port br2 veth2

bridge_add_port_lb br3 link22
bridge_add_port_lb br3 link31
bridge_add_port_lb br3 link51
bridge_add_port_lb br3 veth3

bridge_add_port_lb br4 link32
bridge_add_port_lb br4 link41
bridge_add_port_lb br4 link52
bridge_add_port_lb br4 veth4

#sleeping
sleep $CONVERGENCE_TIME


#testing connectivity
ping_cycle 4
