#!/bin/bash

source "${BASH_SOURCE%/*}/helpers.bash"

function cleanup {
  set +e
  polycubectl ddosmitigator del d1
}
trap cleanup EXIT

echo -e '\nExample 3 \n'
set -e
set -x

create_veth 2

# use ddosmitigator in REDIRECT mode and forward traffic between interfaces

polycubectl ddosmitigator add d1 type=XDP_DRV

#optional debug option, if disabled increase prerformance
# polycubectl ddosmitigator d1 set debug=DEBUG

polycubectl ddosmitigator d1 ports add port1
polycubectl connect d1:port1 eth0

read
polycubectl ddosmitigator d1 blacklist-src del 10.0.0.1
read
polycubectl ddosmitigator d1 blacklist-dst add 10.0.0.1

read
polycubectl ddosmitigator d1 blacklist-dst add 10.0.0.2
read
polycubectl ddosmitigator d1 blacklist-dst add 10.0.0.1
