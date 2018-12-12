#! /bin/bash

# add 1 simplebridge, with N interfaces attached to veth
# M iterations
source "${BASH_SOURCE%/*}/helpers.bash"

N=10
M=3

function cleanup {
  set +e
  del_simplebridges 1
  delete_veth $N
}
trap cleanup EXIT

set -x
set -e

# create_link 10
# create_veth $N
create_veth $N

for k in `seq 1 $M`;
do
  add_simplebridges 2
  for i in `seq 1 $N`;
  do
    simplebridge_add_port br1 veth$i
  done

  set +e
  for i in `seq 1 $N`;
  do
    polycubectl simplebridge br1 ports del port_not_existing
    polycubectl simplebridge br_not_existing ports del port_not_existing
  done
  set -e
  polycubectl simplebridge del br1

  for i in `seq 1 $N`;
  do
    simplebridge_add_port br2 veth$i
  done

  set +e
  for i in `seq 1 $N`;
  do
    polycubectl simplebridge br2 ports del port_not_existing
    polycubectl simplebridge br_not_existing ports del port_not_existing
  done
  set -e
  polycubectl simplebridge del br2
  # del_simplebridges 2
done
