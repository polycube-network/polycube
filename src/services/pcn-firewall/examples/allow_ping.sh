#!/bin/bash

set -x

# assume polycubed is already running
# sudo polycubed -d

# assume standard cube (br) and firewall (fw) already created and running
# ./setup_env.sh

polycubectl fw chain EGRESS insert l4proto=ICMP src=10.0.0.2/32 dst=10.0.0.1 action=ACCEPT

polycubectl fw chain INGRESS insert l4proto=ICMP src=10.0.0.1/32 dst=10.0.0.2 action=ACCEPT

# ARP packets are allowed by default by firewall policy.

echo "Wait for the rules to be updated, and execute ./test_ping.sh"