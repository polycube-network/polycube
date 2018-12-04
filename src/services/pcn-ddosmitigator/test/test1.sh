#!/bin/bash

source "${BASH_SOURCE%/*}/helpers.bash"

function cleanup {
  set +e
  polycubectl detach d1 veth1
  polycubectl ddosmitigator del d1
  sudo ip link del veth1
  sudo ip netns del ns1
}
trap cleanup EXIT

set -e
set -x

#                      ns1
#                  +-----------+
# veth1 <----------|-> veth1_  |
#   ^              +-----------+
#   |
#  ddos

sudo ip netns add ns1
sudo ip link add veth1_ type veth peer name veth1
sudo ip link set veth1_ netns ns1
sudo ip netns exec ns1 ip link set dev veth1_ up
sudo ip link set dev veth1 up
sudo ip netns exec ns1 ifconfig veth1_ 10.0.0.1/24
sudo ifconfig veth1 10.0.0.2/24

TYPE="TC"

if [ -n "$1" ]; then
  TYPE=$1
fi

# use ddosmitigator in REDIRECT mode and forward traffic between interfaces

polycubectl ddosmitigator add d1 type=$TYPE
polycubectl attach d1 veth1

#optional debug option, if disabled increase prerformance
polycubectl ddosmitigator d1 set loglevel=INFO

echo "Show statistics before ping and blacklists"
polycubectl ddosmitigator d1 stats show

echo "Test connectivity"
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -W 2

polycubectl ddosmitigator d1 blacklist-src add 10.0.0.1
echo "This ping should FAIL"
test_fail sudo ip netns exec ns1 ping 10.0.0.2 -c 4 -W 2

polycubectl ddosmitigator d1 show blacklist-src

polycubectl ddosmitigator d1 blacklist-src del 10.0.0.1
echo "This ping should PASS"
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -W 2

echo "Set Debug level to Trace"
polycubectl ddosmitigator d1 set loglevel=TRACE

polycubectl ddosmitigator d1 blacklist-dst add 10.0.0.1
echo "This ping should PASS"
sudo ip netns exec ns1 ping 10.0.0.2 -c 1 -W 2

polycubectl ddosmitigator d1 blacklist-dst add 10.0.0.2
echo "This ping should FAIL"
test_fail sudo ip netns exec ns1 ping 10.0.0.2 -c 5 -W 2

polycubectl ddosmitigator d1 show blacklist-dst

echo "Show statistics after ping and blacklists"
polycubectl ddosmitigator d1 stats show
