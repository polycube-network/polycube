#!/bin/bash

#Argument 1 is the physical interface name
if [ $# -ne 1 ];then
    echo "No arguments <physical_interface_name> supplied"
    exit 1
fi

# assume polycubed is already running
# sudo polycubed -d

# There is no need to run setup_env.sh
# since this example attaches the firewall directly to the physical interface

function fwcleanup {
  set +e
  polycubectl firewall del fw1
}
trap fwcleanup EXIT

echo 'Example using the host mode'

set -e
set -x

polycubectl firewall add fw1
polycubectl firewall fw1 chain INGRESS set default=DROP
polycubectl firewall fw1 chain EGRESS set default=DROP

# Attaching the firewall to the physical interface
polycubectl attach fw1 $1

polycubectl firewall fw1 chain INGRESS rule add 0 l4proto=UDP action=ACCEPT
polycubectl firewall fw1 chain INGRESS rule add 1 l4proto=ICMP action=ACCEPT

polycubectl firewall fw1 chain EGRESS rule add 0 l4proto=UDP action=ACCEPT
polycubectl firewall fw1 chain EGRESS rule add 1 l4proto=ICMP action=ACCEPT

echo "Press any key to test applied rules"
read

#Ping allowed
ping -c 2 google.com

#TCP not allowed (no response)
nping -c 2 google.com
