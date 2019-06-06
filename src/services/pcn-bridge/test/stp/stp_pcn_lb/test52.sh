#! /bin/bash

# 3 pcn-bridge; triangle topology; connect one extra link br1-br2
# test connectivity, after convergence

source "${BASH_SOURCE%/*}/../../helpers.bash"

function cleanup {
  set +e
  del_bridges 1
  del_bridges_lb 2 3
  delete_veth 3
  delete_link 4
}
trap cleanup EXIT

set -x

#setup
create_veth 3
create_link 4

set -e

add_bridges_stp 1
add_bridges_lb 2 3

# create ports
bridge_add_port br1 link11
set_br_priority br1 24576
bridge_add_port br1 link32
bridge_add_port br1 veth1

bridge_add_port_lb br2 link12
set_br_priority_lb br2 28672
bridge_add_port_lb br2 link21
bridge_add_port_lb br2 veth2

bridge_add_port_lb br3 link22
bridge_add_port_lb br3 link31
bridge_add_port_lb br3 veth3

bridge_add_port br1 link41
bridge_add_port_lb br2 link42

#sleeping
sleep $CONVERGENCE_TIME


#testing connectivity
ping_cycle 3
