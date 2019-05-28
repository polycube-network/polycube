#! /bin/bash

# include helper.bash file: used to provide some common function across testing scripts
source "${BASH_SOURCE%/*}/helpers.bash"

# function cleanup: is invoked each time script exit (with or without errors)
function cleanup {
  set +e
  del_bridges 1
  delete_link 2
}
trap cleanup EXIT

# Enable verbose output
set -x

# Makes the script exit, at first error
# Errors are thrown by commands returning not 0 value
set -e

create_link 2
add_bridges 1
bridge_add_port br1 link11
cleanup

create_link 1
add_bridges 1
test_fail bridge_add_port br1 link22
