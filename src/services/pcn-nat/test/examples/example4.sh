#!/usr/bin/env bash

#         TOPOLOGY
#
#       ns1      |                         default ns                       |      ns2
#                |                                                          |
#  +----------+  |  +------------+                          +------------+  |  +----------+
#  |  veth1_  |  |  |   veth1    |  +-----------+--------+  |   veth2    |  |  |  veth2_  |
#  | 10.0.1.1 |-----| 10.0.1.254 |--| router r1 | nat n1 |--| 10.0.2.254 |-----| 10.0.2.1 |
#  +----------+  |  +------------+  +-----------+--------+  +------------+  |  +----------+
#                |                  ^           ^                           |
#                |           r1:to_veth1      r1:to_veth2                   |

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
    polycubectl nat del n1

    delete_veth 2
    sudo pkill -SIGTERM netcat
}
trap cleanup EXIT

ip_veth1_=10.0.1.1
ip_veth2_=10.0.2.1

# When directly connected, the port of the router and the interface of the host
# behave as a unique entity, sharing ip and mac addresses
ip_veth1=10.0.1.254
ip_to_veth1=$ip_veth1
ip_veth2=10.0.2.254
ip_to_veth2=$ip_veth2

missing_ip=10.0.3.1
tcp_int_port=8080
tcp_ext_port=80

set -x

# Create namespaces and veths
create_veth_net 2

# Configure router
polycubectl router add r1
polycubectl router r1 ports add to_veth1 ip=$ip_to_veth1/24 peer=veth1
polycubectl router r1 ports add to_veth2 ip=$ip_to_veth2/24 peer=veth2

# Configure NAT
polycubectl nat add n1
polycubectl attach n1 r1:to_veth2

# Verify that port forwarding does not work before adding the rule
test_tcp_fail

# Add a generic DNAT rule
polycubectl n1 rule dnat append external-ip=$missing_ip internal-ip=$ip_veth1_

# Add a specific Port Forwarding rule to verify priority
polycubectl n1 rule port-forwarding append external-ip=$missing_ip internal-ip=$ip_veth1_ external-port=$tcp_ext_port internal-port=$tcp_int_port

# Verify that Port Forwarding works
test_tcp

# Show natting table
polycubectl n1 natting-table show
