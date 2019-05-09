#!/usr/bin/env bash

timeout=25

function create_veth {
  setup_veth_pair
}

function delete_veth {
  delete_veth_pair
}

function setup_veth_pair {
    sudo ip netns add ns1
    sudo ip link add veth1_ type veth peer name veth1
    sudo ip link set veth1_ netns ns1
    sudo ip netns exec ns1 ip link set dev veth1_ up
    sudo ip netns exec ns1 ifconfig veth1_ 10.0.0.1/24

    sudo ip link set dev veth1 up
    sudo ifconfig veth1 10.0.0.2/24
}

function delete_veth_pair {
  sudo ip link del veth1
  sudo ip netns del ns1
}

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
