#!/bin/bash

set -x

# assume polycubed is already running
# sudo polycubed -d

# assume veth1 and veth2 already created and configured
# ./setup_veth.sh

echo "configure firewall and connect ports"

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

polycubectl firewall fw chain EGRESS rule add 0 l4proto=ICMP src=10.0.0.2/32 dst=10.0.0.1 action=FORWARD

polycubectl firewall fw chain INGRESS rule add 0 l4proto=ICMP src=10.0.0.1 dst=10.0.0.2 action=FORWARD

# ARP packets are allowed by default by firewall policy.

echo "Wait for the rules to be updated, and execute ./test_ping.sh"
