#!/bin/bash

set -x

# assume polycubed is already running
# sudo polycubed -d

# assume veth1 and veth2 already created and configured
# ./setup_veth.sh

echo "Configure firewall and connect ports"

polycubectl firewall add fw

polycubectl firewall fw ports add fw-p1
polycubectl firewall fw ports add fw-p2

polycubectl firewall fw ports fw-p1 set peer=veth1
polycubectl firewall fw ports fw-p2 set peer=veth2

echo "Press any key to set-up rules..."
read

# EGRESS_CHAIN and INGRESS_CHAIN are now considered independently

# veth1 <---- EGRESS ----< veth2
# veth1 >----INGRESS ----> veth2

# allow TCP traffic for test_tcp_adv.sh

polycubectl firewall fw chain EGRESS rule add 0 l4proto=TCP src=10.0.0.0/16 dst=10.0.0.0/16 sport=5678 dport=1234 action=FORWARD tcpflags='SYN, ACK, !RST'
polycubectl firewall fw chain EGRESS rule add 1 l4proto=TCP src=10.0.0.0/16 dst=10.0.0.0/16 sport=5678 dport=1234 action=FORWARD

polycubectl firewall fw chain INGRESS rule add 0 l4proto=TCP src=10.0.0.0/8 dst=10.0.0.0/8 sport=1234 dport=5678 action=FORWARD
polycubectl firewall fw chain INGRESS rule add 1 l4proto=TCP src=10.0.0.0/8 dst=10.0.0.0/8 sport=1234 dport=5678 action=FORWARD tcpflags='SYN, ACK, !RST, !CWR'

echo "Wait for the rules to be updated and launch test_tcp_adv.sh"
