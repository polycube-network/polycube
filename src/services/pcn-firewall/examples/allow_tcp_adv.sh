#!/bin/bash

set -x

# assume polycubed is already running
# sudo polycubed -d

# assume standard cube (br) and firewall (fw) already created and running
# ./setup_env.sh

# allow TCP traffic for test_tcp_adv.sh

polycubectl firewall fw chain EGRESS insert l4proto=TCP src=10.0.0.0/16 dst=10.0.0.0/16 sport=5678 dport=1234 action=FORWARD tcpflags='SYN, ACK, !RST'
polycubectl firewall fw chain EGRESS insert l4proto=TCP src=10.0.0.0/16 dst=10.0.0.0/16 sport=5678 dport=1234 action=FORWARD

polycubectl firewall fw chain INGRESS insert l4proto=TCP src=10.0.0.0/8 dst=10.0.0.0/8 sport=1234 dport=5678 action=FORWARD
polycubectl firewall fw chain INGRESS insert l4proto=TCP src=10.0.0.0/8 dst=10.0.0.0/8 sport=1234 dport=5678 action=FORWARD tcpflags='SYN, ACK, !RST, !CWR'

echo "Wait for the rules to be updated and launch test_tcp_adv.sh"
