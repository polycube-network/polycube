#! /bin/bash

#                 ===packetcapture service test===
#
# Given the configuration of tutorial 1, this test adds a pcn-packetcapture
# instance and attaches it to the "br1: toveth1" interface. After that the traffic
# between the two interfaces of the simplebridge is captured.
# Finally it is checked that the capture dump is stored correctly.

source "${BASH_SOURCE%/*}/../helpers.bash"

set -e
set -x

function cleanup {
  set +e
  delete_veth 2
  echo "FAIL"
}
trap cleanup EXIT

create_veth 2

# namespace ns1 -> veth1 10.0.0.1/24
# namespace ns2 -> veth2 10.0.0.2/24
polycubectl packetcapture add packetcapture_service capture=bidirectional #type=XDP_SKB
polycubectl simplebridge add br1
polycubectl br1 ports add toveth1
polycubectl connect br1:toveth1 veth1
polycubectl br1 ports add toveth2 peer=veth2
sleep 2
polycubectl attach packetcapture_service br1:toveth1

sudo ip netns exec ns1 ping 10.0.0.2 -c 1

sudo rm -f $(polycubectl packetcapture_service show dump | cut -d ' ' -f 4-)


polycubectl detach packetcapture_service br1:toveth1
polycubectl del packetcapture_service
polycubectl del br1
delete_veth 2

set +x
trap - EXIT
echo "SUCCESS"
