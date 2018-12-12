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

echo -e '\nExample using the host mode \n'
echo -e '\n+++ ONLY the ingress chain is supported at the moment! \n'

set -e
set -x

polycubectl firewall add fw

# Connecting the host
polycubectl firewall fw ports add to_host
polycubectl firewall fw ports to_host set peer=:host

# ++ Replace <physicalInterface> with the physical interface name
polycubectl firewall fw ports add to_ens
polycubectl firewall fw ports to_ens set peer=<physicalInterface>

polycubectl firewall fw chain INGRESS rule add 0 l4proto=UDP action=FORWARD
polycubectl firewall fw chain INGRESS rule add 1 l4proto=ICMP action=FORWARD

#ping
ping www.google.it
