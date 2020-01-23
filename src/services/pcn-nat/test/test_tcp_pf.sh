#!/usr/bin/env bash

#         TOPOLOGY
#
#                                  +----------+
#         veth1 (10.0.1.1)---------|    r1    |--------- veth2 (10.0.2.1)
#                               ^  +----------+  ^
#                               |                |
#                             to_veth1        to_veth2 (nat)
#

# TEST 7
# test TCP with netcat and port forwarding

source "${BASH_SOURCE%/*}/helpers.bash"

function test_tcp {
    sudo ip netns exec ns1 netcat -l -w 5 $tcp_int_port&
	sleep 2
	sudo ip netns exec ns2 netcat -w 5 -nvz $missing_ip $tcp_ext_port
	sleep 4
}

function test_tcp_fail {
    sudo ip netns exec ns1 netcat -l -w 5 $tcp_int_port&
	sleep 2
	test_fail sudo ip netns exec ns2 netcat -w 5 -nvz $missing_ip $tcp_ext_port
	sleep 4
}

function cleanup {
    set +e
    delete_veth 2
    polycubectl nat del nat1
    polycubectl router del r1
    sudo pkill -SIGTERM netcat
}
trap cleanup EXIT

veth1_ip=10.0.1.1
veth2_ip=10.0.2.1
to_veth2_ip=10.0.2.254
to_veth1_ip=10.0.1.254
missing_ip=10.0.3.1
tcp_int_port=8080
tcp_ext_port=80
udp_int_port=4000
udp_ext_port=3000

set -x
set -e

create_veth_net 2

polycubectl nat add nat1
polycubectl router add r1

polycubectl router r1 ports add to_veth1 ip=$to_veth1_ip/24 peer=veth1
polycubectl router r1 ports add to_veth2 ip=$to_veth2_ip/24 peer=veth2

polycubectl attach nat1 r1:to_veth2 position=first

test_tcp_fail

polycubectl nat1 rule port-forwarding append external-ip=$missing_ip external-port=$tcp_ext_port internal-ip=$veth1_ip internal-port=$tcp_int_port

test_tcp
