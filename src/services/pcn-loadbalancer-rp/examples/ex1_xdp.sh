#!/usr/bin/env bash

# Create 4 namespaces, one for the client, three for the backends.
# Client is connected to LB, then router, then bridge, which connects the three backends

set -x

# Create namespaces and virtual ethernet interfaces
for i in `seq 1 4`;
do
	sudo ip netns del ns${i} > /dev/null 2>&1	# remove ns if already existed
	sudo ip link del veth${i} > /dev/null 2>&1

	sudo ip netns add ns${i}
	sudo ip link add veth${i}_ type veth peer name veth${i}
	sudo ip link set veth${i}_ netns ns${i}
	sudo ip netns exec ns${i} ip link set dev veth${i}_ up
	sudo ip link set dev veth${i} up
done

set -e

# Set IP addresses and default gateways
sudo ip netns exec ns1 ifconfig veth1_ 10.0.0.1/24
sudo ip netns exec ns1 route add default gw 10.0.0.254 veth1_

sudo ip netns exec ns2 ifconfig veth2_ 10.0.1.2/24
sudo ip netns exec ns3 ifconfig veth3_ 10.0.1.3/24
sudo ip netns exec ns4 ifconfig veth4_ 10.0.1.4/24
sudo ip netns exec ns2 route add default gw 10.0.1.254 veth2_
sudo ip netns exec ns3 route add default gw 10.0.1.254 veth3_
sudo ip netns exec ns4 route add default gw 10.0.1.254 veth4_

# Create polycube services, starting from the LB
# Enabling 'trace' loglevel in LB, so that each received packet is printed on screen
polycubectl lbrp add lb1 loglevel=TRACE type=XDP_SKB

polycubectl lbrp lb1 ports add port1 type=FRONTEND
polycubectl lbrp lb1 ports add port2 type=BACKEND

polycubectl router add r1  type=XDP_SKB
polycubectl router r1 ports add port1 ip=10.0.0.254 netmask=255.255.255.0
polycubectl router r1 ports add port2 ip=10.0.1.254 netmask=255.255.255.0

polycubectl simplebridge add br1  type=XDP_SKB
polycubectl simplebridge br1 ports add port1
polycubectl simplebridge br1 ports add port2
polycubectl simplebridge br1 ports add port3
polycubectl simplebridge br1 ports add port4

polycubectl connect lb1:port1 veth1
polycubectl connect lb1:port2 r1:port1

polycubectl connect br1:port1 r1:port2
polycubectl connect br1:port2 veth2
polycubectl connect br1:port3 veth3
polycubectl connect br1:port4 veth4

# Now we add a service in the LB, then we configure the backends
# We add a single service which features 'ALL' supported protocols
polycubectl lbrp lb1 service add 13.13.13.13 8000 ALL name=test_service

# Instead, backends must be created individually for each protocol;
# the 'ALL' keyword is not allowed here
polycubectl lbrp lb1 service 13.13.13.13 8000 TCP backend add 10.0.1.2 port=8002 name=B1 weight=10
polycubectl lbrp lb1 service 13.13.13.13 8000 UDP backend add 10.0.1.2 port=8002 name=B1 weight=10
# Port is ignored in case of ICMP
polycubectl lbrp lb1 service 13.13.13.13 0 ICMP backend add 10.0.1.2 port=0 name=B1  weight=10

# Create second set of backends
polycubectl lbrp lb1 service 13.13.13.13 8000 TCP backend add 10.0.1.3 port=8003 name=B2  weight=100
polycubectl lbrp lb1 service 13.13.13.13 8000 UDP backend add 10.0.1.3 port=8003 name=B2  weight=100
polycubectl lbrp lb1 service 13.13.13.13 0 ICMP backend add 10.0.1.3 port=8003 name=B2  weight=100

# Create third set of backends
polycubectl lbrp lb1 service 13.13.13.13 8000 TCP backend add 10.0.1.4 port=8004 name=B3
polycubectl lbrp lb1 service 13.13.13.13 8000 UDP backend add 10.0.1.4 port=8004 name=B3
polycubectl lbrp lb1 service 13.13.13.13 0 ICMP backend add 10.0.1.4 port=8004 name=B3


# Now launch the HTTP servers to see if it works
sudo ip netns exec ns2 python -m SimpleHTTPServer 8002 &
sudo ip netns exec ns3 python -m SimpleHTTPServer 8003 &
sudo ip netns exec ns4 python -m SimpleHTTPServer 8004 &

# And test everything by launching a set of client requests
echo "Use the proper commands (e.g.: sudo ip netns exec ns1 curl 13.13.13.13:8000) to test the servers"

# Some examples of possible clients:
#   - get HTTP page from virtual server
#   sudo ip netns exec ns1 curl 13.13.13.13:8000
#
#   - get HTTP page from real server
#   sudo ip netns exec ns1 curl 10.0.1.2:8002
#
#   - get ICMP Echo Reply (PING) from virtual server
#   sudo ip netns exec ns1 ping 13.13.13.13
#
#   - get ICMP Echo Reply (PING) from real server
#   sudo ip netns exec ns1 ping 10.0.1.2


