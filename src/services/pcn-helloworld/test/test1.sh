#! /bin/bash

# include helper.bash file: used to provide some common function across testing scripts
source "${BASH_SOURCE%/*}/helpers.bash"

# function cleanup: is invoked each time script exit (with or without errors)
# please remember to cleanup all entities previously created:
# namespaces, veth, cubes, ..
function cleanup {
  set +e
  polycubectl helloworld del hw1
  delete_veth 2
}
trap cleanup EXIT

# Enable verbose output
set -x

# Makes the script exit, at first error
# Errors are thrown by commands returning not 0 value
set -e

TYPE="TC"

if [ -n "$1" ]; then
  TYPE=$1
fi

# helper.bash function, creates namespaces and veth connected to them
create_veth 2

# create instance of service helloworld
polycubectl helloworld add hw1 type=$TYPE

# add ports to helloworld and connect them to veth(s)
polycubectl helloworld hw1 ports add port1
polycubectl helloworld hw1 ports add port2

polycubectl connect hw1:port1 veth1
polycubectl connect hw1:port2 veth2

# set helloworld action
polycubectl helloworld hw1 set action=FORWARD

# test connectivity between namespace connected by helloworld hw1
# if ping fails, also the script fails (because set -x is enabled)
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -w 2
sudo ip netns exec ns2 ping 10.0.0.1 -c 2 -w 2

# set helloworld action
polycubectl helloworld hw1 set action=SLOWPATH

test_fail sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -w 2
test_fail sudo ip netns exec ns2 ping 10.0.0.1 -c 2 -w 2

# set helloworld action
polycubectl helloworld hw1 set action=DROP

test_fail sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -w 2
test_fail sudo ip netns exec ns2 ping 10.0.0.1 -c 2 -w 2

