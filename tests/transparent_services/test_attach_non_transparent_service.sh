#! /bin/bash

# attach a non transparent service

source "${BASH_SOURCE%/*}/../helpers.bash"

set -e
set -x

function cleanup {
  set +e
  polycubectl del hw0
  polycubectl del hw1
  delete_veth 2
  echo "FAIL"
}
trap cleanup EXIT

create_veth 2

# veth1 -> port1 (hwT) -> hw0 (forward) -> port2 -> veth2
polycubectl helloworld add hw0 action=FORWARD loglevel=TRACE #type=XDP_SKB
polycubectl hw0 ports add port1 peer=veth1
polycubectl hw0 ports add port2 peer=veth2
polycubectl helloworld add hw1 loglevel=TRACE #type=XDP_SKB

if polycubectl attach hw1 hw0:port1; then 
  # we expect an error
  exit 1
fi

polycubectl del hw0
polycubectl del hw1
delete_veth 2

set +x
trap - EXIT
echo "SUCCESS"
