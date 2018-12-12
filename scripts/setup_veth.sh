# This script is used for creating three pairs of veth interfaces
# (vethN <-> vethN_), an endpoint is put in a network namespace nsN
# and is configured with the 10.0.0.N IP.
# It is mainly used for developers for performing quick tests.

#! /bin/bash

set -x

for i in `seq 1 3`;
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
