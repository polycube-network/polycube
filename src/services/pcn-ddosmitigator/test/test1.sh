#!/bin/bash

source "${BASH_SOURCE%/*}/helpers.bash"

function cleanup {
  set +e
  polycubectl ddosmitigator del d1
  delete_veth 2
}
trap cleanup EXIT

set -e
set -x

create_veth 2

TYPE="TC"

if [ -n "$1" ]; then
  TYPE=$1
fi

# use ddosmitigator in REDIRECT mode and forward traffic between interfaces

polycubectl ddosmitigator add d1 type=$TYPE

#optional debug option, if disabled increase prerformance
polycubectl ddosmitigator d1 set loglevel=INFO

polycubectl ddosmitigator d1 ports add port1
polycubectl connect d1:port1 veth1
polycubectl ddosmitigator d1 ports add port2
polycubectl connect d1:port2 veth2

polycubectl ddosmitigator d1 set redirect-port="port2"

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
