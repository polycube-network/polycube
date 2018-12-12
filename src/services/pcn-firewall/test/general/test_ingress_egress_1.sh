#!/bin/bash
# This test changes ingress and egress ports before the rules are configured.

source "${BASH_SOURCE%/*}/../helpers.bash"

function fwcleanup {
  set +e
  polycubectl firewall del fw
  delete_veth 2
}
trap fwcleanup EXIT

echo -e '\nTest Ingress/Egress ports 1 \n'

set -e
set -x

TYPE="TC"

if [ -n "$1" ]; then
  TYPE=$1
fi

create_veth 2

polycubectl firewall add fw type=$TYPE
polycubectl firewall fw set loglevel=DEBUG
polycubectl firewall fw ports add fw-p1
polycubectl firewall fw ports add fw-p2
polycubectl firewall fw ports add fw-p3
polycubectl firewall fw ports add fw-p4
polycubectl firewall fw ports fw-p3 set peer=veth1
polycubectl firewall fw ports fw-p4 set peer=veth2
polycubectl firewall fw set ingress-port=fw-p3
polycubectl firewall fw set egress-port=fw-p4

#INGRESS CHAIN
#matched rules
polycubectl firewall fw chain INGRESS rule add 0 src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=FORWARD

#EGRESS CHAIN
#matched rules
polycubectl firewall fw chain EGRESS rule add 0 src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=FORWARD

#ping

sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1
