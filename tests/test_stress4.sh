#! /bin/bash

# add N simplebridges, with N interfaces attached to veth
# M iterations
source "${BASH_SOURCE%/*}/helpers.bash"

N=10
M=3

function cleanup {
  set +e
  del_simplebridges $N
  delete_veth $N
  # delete_link $N
}
trap cleanup EXIT

set -x
set -e

create_veth $N
# create_link $N

for k in `seq 1 $M`;
do
  add_simplebridges $N
  for i in `seq 1 $N`;
  do
    polycubectl simplebridge br$i ports add veth$i
    polycubectl simplebridge br$i ports veth$i set peer="veth$i"
    polycubectl simplebridge br$i ports add link${i}1
    # simplebridge_add_port br$i link${i}1
    next=$(($i +1))
    if (( $next <= $N )); then
      # simplebridge_add_port br$next link${i}2
      polycubectl simplebridge br$next ports add link${i}2
      polycubectl simplebridge br$i ports link${i}1 set peer="br$next:link${i}2"
      polycubectl simplebridge br$next ports link${i}2 set peer="br$i:link${i}1"
    fi
  done
  sleep 1
  #sleep 40
  #sudo ip netns exec ns1 ping 10.0.0.2 -c 2
  #sudo ip netns exec ns1 ping 10.0.0.7 -c 2
  del_simplebridges $N
done
