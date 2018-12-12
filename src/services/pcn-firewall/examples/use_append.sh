#!/bin/bash

set -x

# assume polycubed is already running
# sudo polycubed -d

# assume veth1 and veth2 already created and configured
# ./setup_veth.sh

function fwcleanup {
  set +e
  polycubectl firewall del fw
}
trap fwcleanup EXIT

echo -e '\nExample appending rules \n'

set -e
set -x

polycubectl firewall add fw
polycubectl firewall fw set loglevel=DEBUG
polycubectl firewall fw ports add fw-p1
polycubectl firewall fw ports add fw-p2
polycubectl firewall fw ports fw-p1 set peer=veth1
polycubectl firewall fw ports fw-p2 set peer=veth2

polycubectl firewall fw chain INGRESS append src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=FORWARD
polycubectl firewall fw chain INGRESS append src=10.0.0.1 dst=10.0.0.2 l4proto=TCP action=DROP

polycubectl firewall fw chain EGRESS append src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=FORWARD
polycubectl firewall fw chain INGRESS append src=10.0.0.1 dst=10.0.0.2 l4proto=TCP action=DROP

#ping
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -w 2
