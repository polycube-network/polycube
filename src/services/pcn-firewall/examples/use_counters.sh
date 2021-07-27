#!/bin/bash

set -x

# assume polycubed is already running
# sudo polycubed -d

# assume standard cube (br) and firewall (fw) already created and running
# ./setup_env.sh

set -e
set -x

echo 'Example showing counters'

polycubectl firewall fw set loglevel=OFF

#add rule to allow ping
polycubectl firewall fw chain INGRESS rule add 0 src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=ACCEPT

polycubectl firewall fw chain EGRESS rule add 0 src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=ACCEPT

#ping (2 packets on each chain)
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -w 2

# Showing counters
polycubectl firewall fw chain INGRESS stats show 0

polycubectl firewall fw chain EGRESS stats 0 show pkts

polycubectl firewall fw chain EGRESS stats 0 show bytes

# Flushing counters of the INGRESS chain
polycubectl firewall fw chain INGRESS reset-counters

