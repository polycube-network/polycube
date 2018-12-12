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
set_br_priority br1 28672
bridge_add_port br1 link42
bridge_add_port br1 veth1

bridge_add_port br2 link12
set_br_priority br2 24576
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

# test ports state
test_forwarding_pcn br1 link11
test_forwarding_pcn br1 link42

test_forwarding_pcn br2 link12
test_forwarding_pcn br2 link21

test_forwarding_lb br3 link22
test_forwarding_lb br3 link31
test_forwarding_lb br3 link51

test_blocking_lb br4 link32
test_forwarding_lb br4 link41
test_blocking_lb br4 link52


# change topology
# polycubectl bridge br4 ports del link52
sudo brctl delif br4 link52
# polycubectl bridge br3 ports del link51
sudo brctl delif br3 link51

#sleeping
sleep $CONVERGENCE_TIME

# test ports state
test_forwarding_pcn br1 link11
test_forwarding_pcn br1 link42

test_forwarding_pcn br2 link12
test_forwarding_pcn br2 link21

test_forwarding_lb br3 link22
test_forwarding_lb br3 link31

test_blocking_lb br4 link32
test_forwarding_lb br4 link41


# change topology
bridge_add_port br2 link51
bridge_add_port_lb br4 link52

#sleeping
sleep $CONVERGENCE_TIME

# test ports state
test_forwarding_pcn br1 link11
test_forwarding_pcn br1 link42

test_forwarding_pcn br2 link12
test_forwarding_pcn br2 link21
test_forwarding_pcn br2 link51

test_forwarding_lb br3 link22

test_forwarding_lb br4 link52
test_blocking_lb br4 link41

macbr4=$(get_bridge_mac_lb br4)
macbr3=$(get_bridge_mac_lb br3)

if [[ $macbr4 < $macbr3 ]]; then
  test_forwarding_lb br4 link32
  test_blocking_lb br3 link31
else
  test_blocking_lb br4 link32
  test_forwarding_lb br3 link31
fi


# change topology
set_br_priority_lb br4 36864

#sleeping
sleep $CONVERGENCE_TIME

# test ports state
test_forwarding_pcn br1 link11
test_forwarding_pcn br1 link42

test_forwarding_pcn br2 link12
test_forwarding_pcn br2 link21
test_forwarding_pcn br2 link51

test_forwarding_lb br3 link22

test_forwarding_lb br4 link52
test_blocking_lb br4 link41

test_blocking_lb br4 link32
test_forwarding_lb br3 link31
