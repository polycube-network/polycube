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
curl -H "Content-Type: application/json" "localhost:9000/polycube/v1/dynmon/dm/dataplane" --upload-file $DIR/test_map_extraction.json
polycubectl dm show

set +e
sudo ip netns exec ns1 ping 10.0.0.2 -c 1 -w 1
set -e

polycubectl dm metrics show

expected_sh='[{"key":{"saddr":1,"daddr":2,"sport":11,"dport":22,"proto":0},"value":0}]'
expected_sa='[{"timestamp":1010,"length":1010}]'
expected_shh='Unhandled Map Type 13 extraction.'
simple_hash_value=$(polycubectl dm metrics SIMPLE_HASH value show)
simple_array_value=$(polycubectl dm metrics SIMPLE_ARRAY value show)
set +e
simple_hash_hash_value=$(polycubectl dm metrics SIMPLE_HASH_OF_HASH value show)
set -e

if [ "$simple_array_value" != "$expected_sa" ]
then
    echo "SIMPLE_ARRAY extraction failed"
    echo "Expected: $expected_sa"
    echo "Got: $simple_array_value"
    exit 1
fi

if [ "$simple_hash_value" != "$expected_sh" ]
then
    echo "SIMPLE_HASH extraction failed"
    echo "Expected: $expected_sh"
    echo "Got: $simple_hash_value"
    exit 1
fi

if [ "$simple_hash_hash_value" != "$expected_shh" ]
then
    echo "SIMPLE_HASH_OF_HASH extraction failed"
    echo "Expected: $expected_shh"
    echo "Got: $simple_hash_hash_value"
    exit 1
fi

echo "All tests passed!"