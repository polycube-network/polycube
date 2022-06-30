#!/bin/bash

# include helper.bash file: used to provide some common function across testing scripts
source "${BASH_SOURCE%/*}/helpers.bash"

function cleanup {
  set +e
  polycubectl lbrp del lb0
  for i in `seq 1 2`; do
          sudo ip link del veth${i}
          sudo ip netns del ns${i}
  done
}
trap cleanup EXIT

set -x
set -e

polycubectl lbrp add lb0 port_mode=SINGLE loglevel=TRACE

for i in `seq 1 2`; do
  create_veth ${i}
  sudo ip netns exec ns${i} ip addr add 192.178.1.${i}/24 dev veth${i}_
  if [[ "$i" -eq 2 ]]; then
    polycubectl lb0 ports add to_ns${i} type=BACKEND peer=veth${i}
  else
    polycubectl lb0 ports add to_ns${i} type=FRONTEND peer=veth${i}
  fi
done

# Add client default route in order to test connectivity
sudo ip netns exec ns1 ip route add default via 192.178.1.2

# Test basic connectivity
sudo ip netns exec ns1 ping 192.178.1.2 -c 2 -w 2
sudo ip netns exec ns2 ping 192.178.1.1 -c 2 -w 2

test_weight

test_service_reachability
