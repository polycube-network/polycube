#!/bin/bash

function cleanup {
  set +e
  polycubectl dettach
  polycubectl detach hwT veth1
  sudo ip link del veth1
  sudo ip netns del ns1
  echo "FAIL"
}
trap cleanup EXIT

set -e
set -x

# create a single veth interface
sudo ip netns add ns1
sudo ip link add veth1_ type veth peer name veth1
sudo ip link set veth1_ netns ns1
sudo ip netns exec ns1 ip link set dev veth1_ up
sudo ip link set dev veth1 up
sudo ip netns exec ns1 ifconfig veth1_ 10.0.0.1/24
sudo ifconfig veth1 10.0.0.2/24

polycubectl transparenthelloworld add hwT loglevel=TRACE #type=XDP_SKB
polycubectl attach hwT veth1

# ping should work
sudo ip netns exec ns1 ping 10.0.0.2 -c 1

polycubectl hwT set egress-action=DROP

if sudo ip netns exec ns1 ping 10.0.0.2 -c 1; then
  # ping should not work
  exit 1
fi

polycubectl detach hwT veth1
polycubectl del hwT
sudo ip link del veth1
sudo ip netns del ns1

set +x
trap - EXIT
echo "SUCCESS"
