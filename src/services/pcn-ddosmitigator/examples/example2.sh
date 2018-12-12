#!/bin/bash

source "${BASH_SOURCE%/*}/helpers.bash"

function cleanup {
  set +e
  polycubectl ddosmitigator del d1
  delete_veth 1
}
trap cleanup EXIT

echo -e '\nExample 2 \n'
set -e
set -x

create_veth 1

# use ddosmitigator in STACK mode
# the user can launch tcpdump to dump traffic and verify rules are working or not

# sudo tcpdump -i veth1 -e -n


polycubectl ddosmitigator add d1

#optional debug option, if disabled increase prerformance
# polycubectl ddosmitigator d1 set debug=DEBUG

polycubectl ddosmitigator d1 ports add port1
polycubectl connect d1:port1 veth1

set +e

#this is to prevent to send arp requests
sudo ip netns exec ns1 sudo arp -s 10.0.0.2 aa:aa:aa:bb:bb:bb

sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -W 2
# can see traffic with tcpdump


polycubectl ddosmitigator d1 blacklist-src add 10.0.0.1
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -W 2
# CANNOT see traffic with tcpdump


polycubectl ddosmitigator d1 blacklist-src del 10.0.0.1
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -W 2
# can see traffic with tcpdump


polycubectl ddosmitigator d1 blacklist-dst add 10.0.0.1
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -W 2
# can see traffic with tcpdump


polycubectl ddosmitigator d1 blacklist-dst add 10.0.0.2
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -W 2
# CANNOT see traffic with tcpdump
