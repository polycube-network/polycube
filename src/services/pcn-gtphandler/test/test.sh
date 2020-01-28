#!/bin/bash 

LIBGTPNL=$HOME/libgtpnl/tools

function cleanup {
  set +e
  set +x

  echo "Cleaning up..."

  polycubectl gh1 del
  sudo ip netns exec ns1 $LIBGTPNL/gtp-link del gtp1 > /dev/null 2>&1
  sudo pkill gtp-link > /dev/null 2>&1
  sudo arp -d 10.0.0.2
  sudo ip route del 172.99.0.1/32 via 10.0.0.2
  sudo ip link del veth1
  sudo ip netns del ns1
}
trap cleanup EXIT

set -x

# Remove ns and veth if already exist
sudo ip netns del ns1 > /dev/null 2>&1
sudo ip link del veth1 > /dev/null 2>&1
sudo ip addr del 172.0.0.1/24 dev lo > /dev/null 2>&1

sudo ip netns add ns1
    
# Configure the UE
sudo ip netns exec ns1 ip addr add 172.99.0.1/32 dev lo
sudo ip netns exec ns1 ip link set lo up

sudo ip link add veth1_ type veth peer name veth1

# Configure the BS
sudo ip link set veth1_ netns ns1
sudo ip netns exec ns1 ip link set dev veth1_ up
sudo ip netns exec ns1 ip addr add 10.0.0.2/24 dev veth1_
BS_MAC=$(sudo ip netns exec ns1 ip addr show veth1_ | grep link/ether | awk -F ' ' '{print $2}')

# Configure the GW
sudo ip link set dev veth1 up
sudo ip addr add 10.0.1.1/24 dev veth1
sudo ip addr add 10.0.0.1/24 dev veth1
sudo ip route add 172.99.0.1/32 via 10.0.0.2
sudo arp -s 10.0.0.2 ${BS_MAC}
    
# Configure the GTP tunnel
sudo ip netns exec ns1 $LIBGTPNL/gtp-link add gtp1 --sgsn &
sleep 1
sudo ip netns exec ns1 $LIBGTPNL/gtp-tunnel add gtp1 v1 100 100 172.99.0.1 10.0.0.1
sudo ip netns exec ns1 route add default dev gtp1

set -e

# Configure the gtphandler
polycubectl gtphandler add gh1
polycubectl attach gh1 veth1
polycubectl gh1 user-equipment add 172.99.0.1 tunnel-endpoint=10.0.0.2 teid=100

ping -I 10.0.1.1 -c 1 172.99.0.1
sudo ip netns exec ns1 ping -c 1 10.0.1.1

set +x

echo "+++ ALL TESTS PASSED +++"