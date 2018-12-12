#!/bin/bash

#cleanup
echo "Cleanup previous config"

sudo ip netns del ns1
sudo ip link del veth1
sudo ip netns del ns2
sudo ip link del veth2

set -e
set -x

echo "Enable IP forwarding"
sudo sysctl -w net.ipv4.ip_forward=1

echo "Checking that IP forwarding is enabled"
echo "/proc/sys/net/ipv4/ip_forward"
cat /proc/sys/net/ipv4/ip_forward

# ns1 (10.0.1.1) -> (10.0.1.254)
# ns2 (10.0.2.1) -> (10.0.2.254)

#create ns
for i in `seq 1 2`;
do
	sudo ip netns add ns${i}
	sudo ip link add veth${i}_ type veth peer name veth${i}
	sudo ip link set veth${i}_ netns ns${i}
	sudo ip netns exec ns${i} ip link set dev veth${i}_ up
	sudo ip link set dev veth${i} up

    sudo ifconfig veth${i} 10.0.${i}.254/24 up

	sudo ip netns exec ns${i} ifconfig veth${i}_ 10.0.${i}.1/24
    sudo ip netns exec ns${i} sudo ip route add default via 10.0.${i}.254

done

