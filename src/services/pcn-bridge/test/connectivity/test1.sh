#! /bin/bash

#test 4 ports access mode

source "${BASH_SOURCE%/*}/../helpers.bash"

N=4
FLAG="TC"

function cleanup {
  set +e
  del_bridges 1
  delete_veth $N
}

if [ $# -gt 2 ]; then
  echo "Wrong number of arguments"
  echo "-D for XDP DRIVER MODE"
  echo "-S for XDP SKB MODE"
  echo "Empty for TC MODE"
  exit 1
fi

if [ -n "$1" ] && [ "$1" != "-D" ] && [ "$1" != "-S" ]; then
  echo "Wrong arguments"
  echo "-D for XDP DRIVER MODE"
  echo "-S for XDP SKB MODE"
  echo "Empty for TC MODE"
  exit 1
fi

if [ "$1" == "-D" ]; then
  FLAG="XDP_DRV"
fi

if [ "$1" == "-S" ]; then
  FLAG="XDP_SKB"
fi

trap cleanup EXIT
set -x

# setup
create_veth $N

set -e

add_bridges 1 $FLAG

for i in `seq 1 $N`;
do
  bridge_add_port br1 veth$i
done

#sleeping
#sleep $CONVERGENCE_TIME
#sleep 40

#testing connectivity
ping_cycle $N
