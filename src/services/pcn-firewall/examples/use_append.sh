#!/bin/bash

set -x

# assume polycubed is already running
# sudo polycubed -d

# assume standard cube (br) and firewall (fw) already created and running
# ./setup_env.sh
#
# assume that no other tests have been run, or the result would be different
# (if you have run ./allow_tcp , then the last comamand would not fail)

echo "Example appending rules"

set -e
set -x

polycubectl firewall fw set loglevel=DEBUG

# allow ICMP traffic and DROP TCP
# from 10.0.0.1 to 10.0.0.2

polycubectl firewall fw chain INGRESS append src=10.0.0.2 dst=10.0.0.1 l4proto=TCP action=DROP

polycubectl firewall fw chain EGRESS append src=10.0.0.1 dst=10.0.0.2 l4proto=TCP action=DROP

polycubectl fw chain EGRESS append l4proto=ICMP src=10.0.0.2/32 dst=10.0.0.1 action=ACCEPT

polycubectl fw chain INGRESS append l4proto=ICMP src=10.0.0.1/32 dst=10.0.0.2 action=ACCEPT

echo "Press any key to test applied rules"
read

#ping
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -w 2

#TCP not allowed (no response)
sudo ip netns exec ns1 nping -c 2 --tcp 10.0.0.2
