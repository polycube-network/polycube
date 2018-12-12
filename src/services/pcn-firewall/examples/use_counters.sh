#!/bin/bash

# assume polycubed is already running
# sudo polycubed -d

# assume veth1 and veth2 already created and configured
# ./setup_veth.sh

function fwcleanup {
  set +e
  polycubectl firewall del fw
}
trap fwcleanup EXIT

echo -e '\Example showing counters \n'

set -x
set -e

polycubectl firewall add fw
polycubectl firewall fw set loglevel=OFF
polycubectl firewall fw ports add fw-p1
polycubectl firewall fw ports add fw-p2
polycubectl firewall fw ports fw-p1 set peer=veth1
polycubectl firewall fw ports fw-p2 set peer=veth2

polycubectl firewall fw chain INGRESS rule add 0 src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=FORWARD

polycubectl firewall fw chain EGRESS rule add 0 src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=FORWARD

#ping (2 packets on each chain)
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -w 2

# Showing counters
polycubectl firewall fw chain INGRESS stats show 0

polycubectl firewall fw chain EGRESS stats 0 show pkts

polycubectl firewall fw chain EGRESS stats 0 show bytes

# Flushing counters of the INGRESS chain
polycubectl firewall fw chain INGRESS reset-counters

