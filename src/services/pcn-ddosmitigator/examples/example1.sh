#!/bin/bash

source "${BASH_SOURCE%/*}/helpers.bash"

function cleanup {
  set +e
  polycubectl ddosmitigator del d1
  delete_veth 2
}
trap cleanup EXIT

echo -e '\nExample 1 \n'
set -e
set -x

create_veth 2

# use ddosmitigator in REDIRECT mode and forward traffic between interfaces

polycubectl ddosmitigator add d1

#optional debug option, if disabled increase prerformance
# polycubectl ddosmitigator d1 set debug=DEBUG

polycubectl ddosmitigator d1 ports add port1
polycubectl connect d1:port1 veth1
polycubectl ddosmitigator d1 ports add port2
polycubectl connect d1:port2 veth2


polycubectl ddosmitigator d1 set redirect-port="port2"

sudo ip netns exec ns1 ping 10.0.0.2 -c 1


polycubectl ddosmitigator d1 blacklist-src add 10.0.0.1

set +e

echo "This ping should FAIL"
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -W 2


polycubectl ddosmitigator d1 blacklist-src del 10.0.0.1
echo "This ping should PASS"
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -W 2


polycubectl ddosmitigator d1 blacklist-dst add 10.0.0.1
echo "This ping should PASS"
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -W 2

polycubectl ddosmitigator d1 blacklist-dst add 10.0.0.2
echo "This ping should FAIL"
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -W 2

