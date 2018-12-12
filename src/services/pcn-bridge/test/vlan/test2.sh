#! /bin/bash

source "${BASH_SOURCE%/*}/../helpers.bash"

FLAG="TC"

function cleanup {
  set +e
  del_bridges 2
  delete_veth 4
  delete_link 1
}

if [ $# -gt 2 ]; then
  echo "Wrong number of arguments"
  echo "-D for XDP DRIVER MODE"
  echo "-S for XDP SKB MODE"
  echo "Empty for TC MODE"
  exit 1
fi

if [ -n "$1" ] && [ "$1" != "-D" ] && [ "$1" != "-S" ]; then
  echo "Wrong arguments"
  echo "-D for XDP DRIVER MODE"
  echo "-S for XDP SKB MODE"
  echo "Empty for TC MODE"
  exit 1
fi

if [ "$1" == "-D" ]; then
  FLAG="XDP_DRV"
fi

if [ "$1" == "-S" ]; then
  FLAG="XDP_SKB"
fi

trap cleanup EXIT
set -x

# setup
create_veth 4
create_link 1

set -e

add_bridges 2 $FLAG

bridge_add_port br1 veth1
bridge_add_port br1 veth2
bridge_add_port br1 link11

bridge_add_port br2 veth3
bridge_add_port br2 veth4
bridge_add_port br2 link12

polycubectl bridge br1 ports veth1 access set vlanid="10"
polycubectl bridge br1 ports veth2 access set vlanid="20"

polycubectl bridge br2 ports veth3 access set vlanid="10"
polycubectl bridge br2 ports veth4 access set vlanid="20"

polycubectl bridge br1 ports link11 set mode="trunk"
polycubectl bridge br2 ports link12 set mode="trunk"

polycubectl bridge br1 ports link11 trunk allowed add 10
polycubectl bridge br1 ports link11 trunk allowed add 20

polycubectl bridge br2 ports link12 trunk allowed add 10
polycubectl bridge br2 ports link12 trunk allowed add 20

#sleeping
#sleep $CONVERGENCE_TIME

echo "should pass"
sudo ip netns exec ns1 ping 10.0.0.3 -c 2 -i 0.5
sudo ip netns exec ns3 ping 10.0.0.1 -c 2 -i 0.5

sudo ip netns exec ns2 ping 10.0.0.4 -c 2 -i 0.5
sudo ip netns exec ns4 ping 10.0.0.2 -c 2 -i 0.5


echo "+++should fail"
test_fail sudo ip netns exec ns1 ping 10.0.0.4 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns4 ping 10.0.0.1 -c 2 -i 0.5 -w 1

test_fail sudo ip netns exec ns2 ping 10.0.0.3 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns3 ping 10.0.0.2 -c 2 -i 0.5 -w 1
