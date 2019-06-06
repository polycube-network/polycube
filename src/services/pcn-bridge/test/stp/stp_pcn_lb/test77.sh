#! /bin/bash

# 5 pcn-bridge; square topology(1,2,3,4) +  triangle(2,3,5) ;
# test connectivity, after convergence
# 1-3 polycube ; 4-5 lb

source "${BASH_SOURCE%/*}/../../helpers.bash"

function cleanup {
  set +e
  del_bridges 3
  del_bridges_lb 4 5
  delete_veth 5
  delete_link 6
}
trap cleanup EXIT

set -x

#setup
create_veth 5
create_link 6

set -e

add_bridges_stp 3
add_bridges_lb 4 5

# create ports
bridge_add_port br1 link11
set_br_priority br1 28672
bridge_add_port br1 link42

bridge_add_port br2 link12
set_br_priority br2 24576
bridge_add_port br2 link21

bridge_add_port br3 link22
set_br_priority br3 32768
bridge_add_port br3 link31

bridge_add_port_lb br4 link32
set_br_priority_lb br4 36864
bridge_add_port_lb br4 link41

bridge_add_port_lb br5 link52
set_br_priority_lb br5 36864
bridge_add_port_lb br5 link61
bridge_add_port br2 link51
bridge_add_port br3 link62

bridge_add_port br1 veth1
bridge_add_port br2 veth2
bridge_add_port br3 veth3
bridge_add_port_lb br4 veth4
bridge_add_port_lb br5 veth5

#sleeping
sleep $CONVERGENCE_TIME


# test ports state
test_forwarding_pcn br1 link11
test_forwarding_pcn br1 link42

test_forwarding_pcn br2 link12
test_forwarding_pcn br2 link21
test_forwarding_pcn br2 link51

test_forwarding_pcn br3 link22
test_forwarding_pcn br3 link31
test_forwarding_pcn br3 link62

test_blocking_lb br4 link32
test_forwarding_lb br4 link41

test_blocking_lb br5 link61
test_forwarding_lb br5 link52
