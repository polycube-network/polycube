#!/bin/bash

source "${BASH_SOURCE%/*}/helpers.bash"

function pbfeanup {
  set +e
  del_pbforwarders 1
  del_veths 2
}
trap pbfeanup EXIT

set -x
set -e

# setup
create_veth 2

add_pbforwarders 1

pbforwarder_add_port pbf1 veth1
pbforwarder_add_port pbf1 veth2

# Add a bunch of not meaningful rules
pbforwarder_add_rules_l2 0 97 pbf1

# Get the MAC addresses of the namespaces
veth1_mac=`LANG=C sudo ip netns exec ns1 ifconfig -a | grep -Po 'ether \K[a-fA-F0-9:]{17}|[a-fA-F0-9]{12}$'`
veth2_mac=`LANG=C sudo ip netns exec ns2 ifconfig -a | grep -Po 'ether \K[a-fA-F0-9:]{17}|[a-fA-F0-9]{12}$'`

# Allows ns1->ns2 ARP Request
polycubectl pbforwarder pbf1 rules add 97 dst_mac=FF:FF:FF:FF:FF:FF action=FORWARD out_port=veth2

polycubectl pbforwarder pbf1 rules add 98 dst_mac=$veth1_mac action=FORWARD out_port=veth1

polycubectl pbforwarder pbf1 rules add 99 dst_mac=$veth2_mac action=FORWARD out_port=veth2

sudo ip netns exec ns1 ping 10.0.0.2 -c 2

