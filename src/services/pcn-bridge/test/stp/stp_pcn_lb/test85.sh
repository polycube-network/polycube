#! /bin/bash

# 4 pcn-bridge; square topology;
# connect extra links between bridges 3-4
# test connectivity, after convergence

source "${BASH_SOURCE%/*}/../../helpers.bash"

function cleanup {
  set +e
  del_bridges 2
  del_bridges_ovs 3 4
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
add_bridges_ovs 3 4

# create ports
bridge_add_port br1 link11
bridge_add_port br1 link42
bridge_add_port br1 veth1
bridge_add_port br1 link51

bridge_add_port br2 link12
bridge_add_port br2 link21
bridge_add_port br2 veth2
bridge_add_port br2 link52

bridge_add_port_ovs br3 link22
bridge_add_port_ovs br3 link31
bridge_add_port_ovs br3 veth3
set_br_priority_ovs br3 28672

bridge_add_port_ovs br4 link32
bridge_add_port_ovs br4 link41
bridge_add_port_ovs br4 veth4
set_br_priority_ovs br4 24576

#sleeping
sleep $CONVERGENCE_TIME

# test ports state
test_forwarding_pcn br1 link11
test_forwarding_pcn br1 link42
test_forwarding_pcn br1 link51

test_blocking_pcn br2 link12
test_forwarding_pcn br2 link21
test_blocking_pcn br2 link52

test_forwarding_ovs br3 link22
test_forwarding_ovs br3 link31

test_forwarding_ovs br4 link41
test_forwarding_ovs br4 link32


# change topology
polycubectl br1 ports del link51
polycubectl br2 ports del link52

#sleeping
sleep $CONVERGENCE_TIME

# test ports state
test_forwarding_pcn br1 link11
test_forwarding_pcn br1 link42

test_blocking_pcn br2 link12
test_forwarding_pcn br2 link21

test_forwarding_ovs br3 link22
test_forwarding_ovs br3 link31

test_forwarding_ovs br4 link32
test_forwarding_ovs br4 link41


# change topology
bridge_add_port br2 link51
bridge_add_port_ovs br4 link52

#sleeping
sleep $CONVERGENCE_TIME

# test ports state
test_forwarding_pcn br1 link42

test_blocking_pcn br2 link21
test_forwarding_pcn br2 link51

test_forwarding_ovs br3 link22
test_forwarding_ovs br3 link31

test_forwarding_ovs br4 link32
test_forwarding_ovs br4 link52
test_forwarding_ovs br4 link41

macbr1=$(get_bridge_mac_pcn br1)
macbr2=$(get_bridge_mac_pcn br2)

if [[ $macbr1 < $macbr2 ]]; then
  test_forwarding_pcn br1 link11
  test_blocking_pcn br2 link12
else
  test_blocking_pcn br1 link11
  test_forwarding_pcn br2 link12
fi


# change topology
set_br_priority br2 36864

#sleeping
sleep $CONVERGENCE_TIME

# test ports state
test_forwarding_pcn br1 link11
test_forwarding_pcn br1 link42

test_blocking_pcn br2 link12
test_blocking_pcn br2 link21
test_forwarding_pcn br2 link51

test_forwarding_ovs br3 link22
test_forwarding_ovs br3 link31

test_forwarding_ovs br4 link52
test_forwarding_ovs br4 link41
test_forwarding_ovs br4 link32
