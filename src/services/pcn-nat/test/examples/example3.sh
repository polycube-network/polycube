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

# DESTINATION NAT EXAMPLE

source "../helpers.bash"

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

# Verify that PING does not work before setting the rule
test_fail sudo ip netns exec ns2 ping $missing_ip -c 3

# Add the rule
polycubectl n1 rule dnat append external-ip=$missing_ip internal-ip=$ip_veth1_

# Verify that PING now works
sudo ip netns exec ns2 ping $missing_ip -c 3

# Show natting table
polycubectl n1 natting-table show
