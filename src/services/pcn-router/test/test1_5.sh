#! /bin/bash
# 			  TOPOLOGY
#
#
#
#                         +------+----------------------------> (up to vethN)
#             veth1 ------|  r1  |------- veth2
#                         +------
#                             |
#                             |
#                             |
#                           veth3


source "${BASH_SOURCE%/*}/helpers.bash"

N=5

function cleanup {
  set +e
  del_routers 1
  delete_veth 5
}
trap cleanup EXIT

set -x
create_veth_net 5

set -e

add_routers 1

for i in `seq 1 $N`;
  do
    router_add_port_as_gateway r1 veth$i $i
  done

ping_cycle $N
