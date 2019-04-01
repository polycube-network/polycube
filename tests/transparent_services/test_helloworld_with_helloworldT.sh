#! /bin/bash

# helloworld with a transparent cube service

source "${BASH_SOURCE%/*}/../helpers.bash"

set -e
set -x

function cleanup {
  set +e
  polycubectl del hw0
  polycubectl del hwT
  delete_veth 2
  echo "FAIL"
}
trap cleanup EXIT

create_veth 2

# veth1 -> port1 (hwT) -> hw0 (forward) -> port2 -> veth2
polycubectl helloworld add hw0 action=FORWARD loglevel=TRACE #type=XDP_SKB
polycubectl hw0 ports add port1 peer=veth1
polycubectl hw0 ports add port2 peer=veth2
polycubectl transparenthelloworld add hwT loglevel=TRACE #type=XDP_SKB
polycubectl attach hwT hw0:port1

sudo ip netns exec ns1 ping 10.0.0.2 -c 1

polycubectl del hw0
polycubectl del hwT
delete_veth 2

set +x
trap - EXIT
echo "SUCCESS"
