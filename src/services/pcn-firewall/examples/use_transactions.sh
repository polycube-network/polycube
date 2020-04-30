#!/bin/bash

set -x

# assume polycubed is already running
# sudo polycubed -d

# assume standard cube (br) and firewall (fw) already created and running
# ./setup_env.sh

echo 'Example using transactions'

set -e
set -x

echo 'Activating the transaction mode'
polycubectl firewall fw set interactive=false

polycubectl firewall fw chain INGRESS append src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=FORWARD
polycubectl firewall fw chain INGRESS append src=10.0.0.1 dst=10.0.0.2 l4proto=TCP action=DROP
# Apply ingress rules
polycubectl firewall fw chain INGRESS apply-rules

polycubectl firewall fw chain EGRESS append src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=FORWARD
polycubectl firewall fw chain EGRESS append src=10.0.0.1 dst=10.0.0.2 l4proto=TCP action=DROP
# Apply egress rules
polycubectl firewall fw chain EGRESS apply-rules

#ping
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -w 2
