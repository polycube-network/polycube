#!/bin/bash

# include helper.bash file: used to provide some common function across testing scripts
source "${BASH_SOURCE%/*}/helpers.bash"

function cleanup {
  set +e
  polycubectl lbrp del lb0
  for i in `seq 1 3`; do
          sudo ip link del veth${i}
          sudo ip netns del ns${i}
  done
}
trap cleanup EXIT

set -x
set -e

polycubectl lbrp add lb0 port_mode=MULTI loglevel=TRACE

for i in `seq 1 3`; do
  create_veth ${i}

  if [[ "$i" -eq 3 ]]; then
    sudo ip netns exec ns${i} ip addr add 192.178.1.254/24 dev veth${i}_
    polycubectl lb0 ports add to_ns${i} type=BACKEND peer=veth${i}
  else
    sudo ip netns exec ns${i} ip addr add 192.178.1.${i}/32 dev veth${i}_ peer 192.178.1.254/32
    sudo ip netns exec ns${i} ip route add default via 192.178.1.254
    polycubectl lb0 ports add to_ns${i} type=FRONTEND peer=veth${i} ip=192.178.1.${i}
  fi
done


# Test basic connectivity
sudo ip netns exec ns1 ping 192.178.1.254 -c 2 -w 2
sudo ip netns exec ns2 ping 192.178.1.254 -c 2 -w 2
sudo ip netns exec ns3 ping 192.178.1.1 -c 2 -w 2
sudo ip netns exec ns3 ping 192.178.1.2 -c 2 -w 2

test_weight

test_service_reachability
