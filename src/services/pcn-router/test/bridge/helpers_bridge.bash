#!/usr/bin/env bash

# default const value
CONVERGENCE_TIME="50"

function test_fail {
  set +e
  res=$($@)
  local status=$?
  set -e
  if [ $status -ne 0 ]; then
    return 0
  else
    return 1
  fi
}

function get_bridge_mac_pcn {
    address=$(polycubectl simplebridge $1 stp 1 show address)
    echo $(echo $address | tr -d ':"')
}

function get_bridge_mac_lb {
    address=$(brctl showstp $1)
    echo $(echo $stp | awk '{print $4}' | cut -d. -f2)
}

function set_br_priority {
  polycubectl simplebridge $1 stp 1 set priority=$2
}

function set_br_priority_lb {
  sudo brctl setbridgeprio $1 $2
}

function set_br_priority_ovs {
  sudo ovs-vsctl set Bridge $1 other_config:stp-priority=$2
}

function add_bridges_ovs {
  for i in `seq $1 $2`;
  do
    sudo ovs-vsctl add-br br$i
    sudo ovs-vsctl set Bridge br$i stp_enable=true
  done
}

function add_bridges_lb {
  for i in `seq $1 $2`;
  do
    sudo brctl addbr br$i
    sudo brctl stp br$i on
    sudo ip link set br$i up
  done
}

function add_bridges {
  for i in `seq 1 $1`;
  do
    polycubectl simplebridge add br$i
  done
}

function del_bridges_ovs {
  for i in `seq $1 $2`;
  do
    sudo ovs-vsctl del-br br$i
  done
}

function del_bridges_lb {
  for i in `seq $1 $2`;
  do
    sudo ip link set br$i down
    sudo brctl delbr br$i
  done
}

function del_bridges {
  for i in `seq 1 $1`;
  do
    polycubectl simplebridge del br$i
  done
}

function bridge_add_port {
  polycubectl simplebridge $1 ports add $2
  polycubectl simplebridge $1 ports $2 set peer=$2
}

function bridge_add_port_lb {
  sudo brctl addif $1 $2
}

function bridge_add_port_ovs {
  sudo ovs-vsctl add-port $1 $2
}




function test_forwarding_ovs {
  test_forward echo $(sudo ovs-ofctl show $1 | grep -A 2 $2 | grep state | awk  '{print $2}')
}

function test_forwarding_lb {
  test_forwarding echo $(brctl showstp $1 | grep -A 1 $2 | grep state | awk  '{print $5}')
}

function test_forwarding_pcn {
  test_forwarding polycubectl simplebridge $1 ports $2 stp 1 show state
}

function test_forwarding {
  res=$($@)
  local status=$?
  if [ $status -ne 0 ]; then
    return $status
  else
		echo "$res"
		if [[ "$res"  == *"forwarding"* ]]; then
			return $status
		else
			echo "expexted port state forwarding"
			return 1
		fi
  fi
  return $status
}

function test_forward {
  res=$($@)
  local status=$?
  if [ $status -ne 0 ]; then
    return $status
  else
		echo "$res"
		if [[ "$res"  == *"FORWARD"* ]]; then
			return $status
		else
			echo "expexted port state forwarding"
			return 1
		fi
  fi
  return $status
}

function test_blocking_ovs {
  test_block echo $(sudo ovs-ofctl show $1 | grep -A 2 $2 | grep state | awk  '{print $2}')
}

function test_blocking_lb {
  test_blocking echo $(brctl showstp $1 | grep -A 1 $2 | grep state | awk  '{print $5}')
}

function test_blocking_pcn {
  test_blocking polycubectl simplebridge $1 ports $2 stp 1 show state
}

function test_blocking {
  res=$($@)
  local status=$?
  if [ $status -ne 0 ]; then
    return $status
  else
		echo "$res"
		if [[ "$res"  == *"blocking"* ]]; then
			return $status
		else
			echo "expexted port state blocking"
			return 1
		fi
  fi
  return $status
}

function test_block {
  res=$($@)
  local status=$?
  if [ $status -ne 0 ]; then
    return $status
  else
		echo "$res"
		if [[ "$res"  == *"BLOCK"* ]]; then
			return $status
		else
			echo "expexted port state blocking"
			return 1
		fi
  fi
  return $status
}
