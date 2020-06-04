#! /bin/bash

# include helper.bash file: used to provide some common function across testing scripts
source "${BASH_SOURCE%/*}/helpers.bash"

# function cleanup: is invoked each time script exit (with or without errors)
# please remember to cleanup all entities previously created:
# namespaces, veth, cubes, ..
function cleanup {
  set +e
  polycubectl dynmon del dm
  delete_veth 2
}
trap cleanup EXIT

# Enable verbose output
set -x

# Makes the script exit, at first error
# Errors are thrown by commands returning not 0 value
set -e

DIR=$(dirname "$0")

TYPE="TC"

if [ -n "$1" ]; then
  TYPE=$1
fi

# helper.bash function, creates namespaces and veth connected to them
create_veth 2

# create instance of service dynmon
polycubectl dynmon add dm type=$TYPE
polycubectl dm show

# attaching the monitor to veth1
polycubectl attach dm veth1



# injecting a dataplane configuration
curl -H "Content-Type: application/json" "localhost:9000/polycube/v1/dynmon/dm/dataplane" --upload-file $DIR/test_dataplane.json
polycubectl dm show

set +e
ping_result=$(sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -w 2 | grep -Po '[0-9]+(?= +packets transmitted)')
set -e

polycubectl dm metrics show
polycubectl dm open-metrics show
polycubectl dm metrics packets_total show

metric_value=$(polycubectl dm metrics packets_total value show | sed -n "s/^\[\([0-9]*\)\]$/\1/p")

if [ $metric_value -eq 0 ] || [ $metric_value -lt $ping_result ];
then 
    echo "Error: expected packets_value > 0 and >= $ping_result"
    echo "packets_total: $packets_total"
    exit 1
else
    echo "packets_total: $packets_total"
    echo "TEST: OK"
fi;