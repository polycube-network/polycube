#! /bin/bash
# 			    TOPOLOGY
#	+------+				+------+
#	|  r1  |--------------------------------|  r2  |
#	+------+ p1:10.0.0.1	    p2:10.0.0.2 +------+
#
#

source "${BASH_SOURCE%/*}/../helpers.bash"

function cleanup {
  set +e
  del_routers 2
}
trap cleanup EXIT

set -x
set -e
# Create the routers and connect them
polycubectl router add r1 shadow=true
polycubectl router add r2 # the router r2 can also be shadow, it is the same
connect_router_p_to_p r1 r2 0

# Ping from inside r1 to r2
sudo ip netns exec pcn-r1 ping 10.0.0.2 -c 3 -i 0.5
