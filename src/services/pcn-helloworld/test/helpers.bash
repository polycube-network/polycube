#!/usr/bin/env bash

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

function create_veth {
  for i in `seq 1 $1`;
  do
  	sudo ip netns add ns${i}
  	sudo ip link add veth${i}_ type veth peer name veth${i}
  	sudo ip link set veth${i}_ netns ns${i}
  	sudo ip netns exec ns${i} ip link set dev veth${i}_ up
  	sudo ip link set dev veth${i} up
  	sudo ip netns exec ns${i} ifconfig veth${i}_ 10.0.0.${i}/24
  done
}

function create_link {
  for i in `seq 1 $1`;
  do
  	sudo ip link add link${i}1 type veth peer name link${i}2
  	sudo ip link set dev link${i}1 up
  	sudo ip link set dev link${i}2 up
  done
}

function delete_veth {
  for i in `seq 1 $1`;
  do
  	sudo ip link del veth${i}
  	sudo ip netns del ns${i}
  done
}

function delete_link {
  for i in `seq 1 $1`;
  do
  	sudo ip link del link${i}1
  done
}
