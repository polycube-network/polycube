#! /bin/bash

# multiple services (position = last)
# TODO: check that the services are actually transverse on the same order

source "${BASH_SOURCE%/*}/../helpers.bash"

set -e
set -x

function cleanup {
  set +e
  polycubectl del hw0
  polycubectl del hwTA
  polycubectl del hwTB
  polycubectl del hwTC
  polycubectl del hwTD
  delete_veth 2
  echo "FAIL"
}
trap cleanup EXIT

create_veth 2

polycubectl helloworld add hw0 action=FORWARD loglevel=TRACE #type=XDP_SKB
polycubectl hw0 ports add port1 peer=veth1
polycubectl hw0 ports add port2 peer=veth2

polycubectl transparenthelloworld add hwTA loglevel=TRACE #type=XDP_SKB
polycubectl transparenthelloworld add hwTB loglevel=TRACE #type=XDP_SKB
polycubectl transparenthelloworld add hwTC loglevel=TRACE #type=XDP_SKB
polycubectl transparenthelloworld add hwTD loglevel=TRACE #type=XDP_SKB

polycubectl attach hwTA hw0:port1 position=last
polycubectl attach hwTB hw0:port1 position=last
polycubectl attach hwTC hw0:port1 position=last
polycubectl attach hwTD hw0:port1 position=last

sudo ip netns exec ns1 ping 10.0.0.2 -c 1

# must print
# ingress: D -> C -> B -> A
# egress: A -> B -> C -> D

polycubectl del hwTA
polycubectl del hwTB
polycubectl del hwTC
polycubectl del hwTD

sudo ip netns exec ns1 ping 10.0.0.2 -c 1

polycubectl del hw0
delete_veth 2

set +x
trap - EXIT
echo "SUCCESS"
