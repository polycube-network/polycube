#! /bin/bash

source "${BASH_SOURCE%/*}/helpers.bash"
function cleanup {
  set +e
  polycubectl simplebridge del br1
  delete_veth 2
}
trap cleanup EXIT

set -x
set -e

TYPE="TC"

if [ -n "$1" ]; then
  TYPE=$1
fi

create_veth 2

polycubectl simplebridge add br1 type=$TYPE

polycubectl simplebridge br1 ports add port1
polycubectl simplebridge br1 ports add port2

polycubectl connect br1:port1 veth1
polycubectl connect br1:port2 veth2

sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -w 2
sudo ip netns exec ns2 ping 10.0.0.1 -c 2 -w 2

sleep 3
