#!/usr/bin/env bash

#         TOPOLOGY
#
#                                  +----------+
#         veth1 (10.0.1.1)---------|    r1    |--------- veth2 (10.0.2.1)
#                               ^  +----------+  ^
#                               |                |
#                             to_veth1        to_veth2 (nat)
#

# TEST 10
# test UDP with iperf and destination nat

source "${BASH_SOURCE%/*}/helpers.bash"

function test_udp {
	npingOutput="$(sudo ip netns exec ns2 nping --udp -c 1 -p $udp_port --source-port 50000 $missing_ip)"
    if [[ $npingOutput != *"Rcvd: 1"* ]]; then
        exit 1
    fi
}

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
missing_ip=10.0.3.1
udp_port=3000

set -x
set -e

create_veth_net 2

polycubectl nat add nat1
polycubectl router add r1

polycubectl router r1 ports add to_veth1 ip=$to_veth1_ip netmask=255.255.255.0 peer=veth1
polycubectl router r1 ports add to_veth2 ip=$to_veth2_ip netmask=255.255.255.0 peer=veth2

polycubectl attach nat1 r1:to_veth2 position=first

test_fail test_udp

# Add rule for the ICMP Destination Unreachable packet that comes back
polycubectl nat1 rule snat append internal-net=$veth1_ip/32 external-ip=$missing_ip

# Add rule for the UDP packet
polycubectl nat1 rule dnat append external-ip=$missing_ip internal-ip=$veth1_ip

test_udp
