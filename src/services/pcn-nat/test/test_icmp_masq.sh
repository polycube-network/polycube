#!/usr/bin/env bash

#         TOPOLOGY
#
#                                  +----------+
#         veth1 (10.0.1.1)---------|    r1    |--------- veth2 (10.0.2.1)
#                               ^  +----------+  ^
#                               |                |
#                             to_veth1        to_veth2 (nat)
#

# TEST 2
# test ping with masquerade

source "${BASH_SOURCE%/*}/helpers.bash"

function cleanup {
    set +e
    delete_veth 2
    polycubectl nat del nat1
    polycubectl router del r1
}
trap cleanup EXIT

veth1_ip=10.0.1.1
veth2_ip=10.0.2.1
to_veth2_ip=10.0.2.254
to_veth1_ip=10.0.1.254

set -x
set -e

create_veth_net 2

polycubectl nat add nat1
polycubectl router add r1

polycubectl router r1 ports add to_veth1 ip=$to_veth1_ip netmask=255.255.255.0 peer=veth1
polycubectl router r1 ports add to_veth2 ip=$to_veth2_ip netmask=255.255.255.0 peer=veth2

polycubectl attach nat1 r1:to_veth2 position=first

polycubectl nat1 rule masquerade enable

sudo ip netns exec ns1 ping $veth2_ip -c 3
