#! /bin/bash

set -x

# Setup veths. Useful for testing service with linux namespaces.

echo "Configuring network namespaces"

for i in `seq 1 2`;
do
	sudo ip netns del ns${i} > /dev/null 2>&1	# remove ns if already existed
	sudo ip link del veth${i} > /dev/null 2>&1

	sudo ip netns add ns${i}
	sudo ip link add veth${i}_ type veth peer name veth${i}
	sudo ip link set veth${i}_ netns ns${i}
	sudo ip netns exec ns${i} ip link set dev veth${i}_ up
	sudo ip link set dev veth${i} up
	sudo ip netns exec ns${i} ifconfig veth${i}_ 10.0.0.${i}/24
done

# Setup standard cube (Simplebridge)

echo "Configuring standard cube"

polycubectl br del

polycubectl simplebridge add br

polycubectl simplebridge br ports add port1
polycubectl simplebridge br ports add port2

polycubectl connect br:port1 veth1
polycubectl connect br:port2 veth2

# Creating and attaching Firewall to Simplebridge

echo "Configuring Firewall"

polycubectl fw del 

polycubectl firewall add fw
polycubectl attach fw br:port1
polycubectl firewall fw chain INGRESS set default=DROP
polycubectl firewall fw chain EGRESS set default=DROP

# EGRESS_CHAIN and INGRESS_CHAIN are now considered independently

# br:port1 <---- EGRESS ----< br:port2
# br:port1 >----INGRESS ----> br:port2
