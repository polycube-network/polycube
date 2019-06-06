#! /bin/bash

source "${BASH_SOURCE%/*}/../helpers.bash"

FLAG="TC"

function cleanup {
  set +e
  del_bridges 3
  delete_veth 13
  delete_link 2
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
create_veth 13
create_link 2

set -e

add_bridges 3 $FLAG

bridge_add_port br1 veth1
bridge_add_port br1 veth2
bridge_add_port br1 veth3
bridge_add_port br1 veth4
bridge_add_port br1 link11

bridge_add_port br2 veth5
bridge_add_port br2 veth6
bridge_add_port br2 veth7
bridge_add_port br2 veth8
bridge_add_port br2 veth9

bridge_add_port br2 link12
bridge_add_port br2 link21

bridge_add_port br3 veth10
bridge_add_port br3 veth11
bridge_add_port br3 veth12
bridge_add_port br3 veth13
bridge_add_port br3 link22

polycubectl br1 ports veth1 access set vlanid="10"
polycubectl br1 ports veth2 access set vlanid="20"
polycubectl br1 ports veth3 access set vlanid="20"
polycubectl br1 ports veth4 access set vlanid="30"

polycubectl br2 ports veth5 access set vlanid="10"
polycubectl br2 ports veth6 access set vlanid="30"
polycubectl br2 ports veth7 access set vlanid="40"
polycubectl br2 ports veth8 access set vlanid="50"
polycubectl br2 ports veth9 access set vlanid="50"


polycubectl br3 ports veth10 access set vlanid="10"
polycubectl br3 ports veth11 access set vlanid="40"
polycubectl br3 ports veth12 access set vlanid="60"
polycubectl br3 ports veth13 access set vlanid="60"


polycubectl br1 ports link11 set mode="trunk"
polycubectl br2 ports link12 set mode="trunk"
polycubectl br2 ports link21 set mode="trunk"
polycubectl br3 ports link22 set mode="trunk"


polycubectl br1 ports link11 trunk allowed add 10
polycubectl br1 ports link11 trunk allowed add 30
polycubectl br2 ports link12 trunk allowed add 10
polycubectl br2 ports link12 trunk allowed add 30

polycubectl br2 ports link21 trunk allowed add 10
polycubectl br2 ports link21 trunk allowed add 40
polycubectl br3 ports link22 trunk allowed add 10
polycubectl br3 ports link22 trunk allowed add 40

#sleeping
# sleep $CONVERGENCE_TIME

echo ""
echo "++should pass++"
echo "++local bridge vlans++"
sudo ip netns exec ns2 ping 10.0.0.3 -c 2 -i 0.5
sudo ip netns exec ns3 ping 10.0.0.2 -c 2 -i 0.5

sudo ip netns exec ns8 ping 10.0.0.9 -c 2 -i 0.5
sudo ip netns exec ns9 ping 10.0.0.8 -c 2 -i 0.5

sudo ip netns exec ns12 ping 10.0.0.13 -c 2 -i 0.5
sudo ip netns exec ns13 ping 10.0.0.12 -c 2 -i 0.5

echo ""
echo "++should pass++"
echo "++vlan10 over all switches++"

sudo ip netns exec ns1 ping 10.0.0.5 -c 2 -i 0.5
sudo ip netns exec ns1 ping 10.0.0.10 -c 2 -i 0.5

sudo ip netns exec ns5 ping 10.0.0.1 -c 2 -i 0.5
sudo ip netns exec ns5 ping 10.0.0.10 -c 2 -i 0.5

sudo ip netns exec ns10 ping 10.0.0.1 -c 2 -i 0.5
sudo ip netns exec ns10 ping 10.0.0.5 -c 2 -i 0.5

echo ""
echo "++should pass++"
echo "++over 2 bridges++"
sudo ip netns exec ns4 ping 10.0.0.6 -c 2 -i 0.5
sudo ip netns exec ns6 ping 10.0.0.4 -c 2 -i 0.5

sudo ip netns exec ns7 ping 10.0.0.11 -c 2 -i 0.5
sudo ip netns exec ns11 ping 10.0.0.7 -c 2 -i 0.5

echo ""
echo "++should FAIL++"
echo "++should FAIL++"
test_fail sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns1 ping 10.0.0.3 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns1 ping 10.0.0.4 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns1 ping 10.0.0.6 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns1 ping 10.0.0.7 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns1 ping 10.0.0.8 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns1 ping 10.0.0.9 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns4 ping 10.0.0.11 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns4 ping 10.0.0.12 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns4 ping 10.0.0.13 -c 2 -i 0.5 -w 1

test_fail sudo ip netns exec ns2 ping 10.0.0.1 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns2 ping 10.0.0.4 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns2 ping 10.0.0.5 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns2 ping 10.0.0.6 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns2 ping 10.0.0.7 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns2 ping 10.0.0.8 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns2 ping 10.0.0.9 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns2 ping 10.0.0.10 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns2 ping 10.0.0.11 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns2 ping 10.0.0.12 -c 2 -i 0.5 -w 1
test_fail sudo ip netns exec ns2 ping 10.0.0.13 -c 2 -i 0.5 -w 1
