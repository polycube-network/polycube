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
}
trap cleanup EXIT

set -x
set -e

create_veth $N

for k in `seq 1 $M`;
do
  add_simplebridges $N
  for i in `seq 1 $N`;
  do
    polycubectl simplebridge br$i ports add veth$i
  done
  del_simplebridges $N
done
