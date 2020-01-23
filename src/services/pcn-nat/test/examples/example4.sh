#!/usr/bin/env bash

#         TOPOLOGY
#
#                              +-----------+                 +-----------+
#      veth1 (10.0.1.1)--------|     r1    |-----------------|    nat1   |------- veth2 (10.0.2.1)
#                            ^ +-----------+  ^            ^ +-----------+  ^
#                            |                |            |                |
#                          to_veth1        to_nat1  to_r1 (INTERNAL)    to_veth2 (EXTERNAL)
#

# PORT FORWARDING EXAMPLE

source "../helpers.bash"

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
    polycubectl router del r1
    polycubectl nat del nat1

    delete_veth 2
    sudo pkill -SIGTERM netcat
}
trap cleanup EXIT

veth1_ip=10.0.1.1
veth2_ip=10.0.2.1
to_nat_ip=10.0.2.254
to_veth1_ip=10.0.1.254
missing_ip=10.0.3.1
tcp_int_port=8080
tcp_ext_port=80

set -x

# Create namespaces and veths
create_veth_net 2

# Configure NAT and router
polycubectl nat add nat1
polycubectl router add r1

polycubectl router r1 ports add to_veth1 ip=$to_veth1_ip/24
polycubectl router r1 ports to_veth1 set peer=veth1
polycubectl router r1 ports add to_nat1 ip=$to_nat_ip/24

polycubectl nat1 ports add to_r1 type=INTERNAL
polycubectl connect nat1:to_r1 r1:to_nat1

polycubectl nat1 ports add to_veth2 type=EXTERNAL ip=$to_nat_ip
polycubectl connect nat1:to_veth2 veth2

# Verify that port forwarding does not work before adding the rule
test_tcp_fail

# Add a generic DNAT rule
polycubectl nat1 rule dnat append external-ip=$missing_ip internal-ip=$veth1_ip

# Add a specific Port Forwarding rule to verify priority
polycubectl nat1 rule port-forwarding append external-ip=$missing_ip internal-ip=$veth1_ip external-port=$tcp_ext_port internal-port=$tcp_int_port

# Verify that Port Forwarding works
test_tcp

# Show natting table
polycubectl nat1 natting-table show
