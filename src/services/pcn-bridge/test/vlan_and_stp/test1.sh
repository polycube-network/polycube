#!/bin/bash

source "${BASH_SOURCE%/*}/../helpers.bash"

loglevel="info"

function cleanup {
  echo "cleaning up.."
  set +e
  polycubectl b1 del
  polycubectl b2 del
  polycubectl b3 del
  delete_veth 3
}
trap cleanup EXIT

function verifyForwarding {
  if [ $1 = "forwarding" ]; then
    return 0
  else
    return 1
  fi
}

function verifyBlocking {
  if [ $1 = "blocking" ]; then
    return 0
  else
    return 1
  fi
}

#setup link and service
echo "setting up topology.."

set -e

create_veth 3

#setup topology
polycubectl bridge add b1 loglevel=$loglevel stp-enabled=true
polycubectl bridge add b2 loglevel=$loglevel stp-enabled=true
polycubectl bridge add b3 loglevel=$loglevel stp-enabled=true

polycubectl b1 ports add tov1 peer=veth1
polycubectl b1 ports tov1 access set vlanid=14
polycubectl b1 ports add tob2 mode=trunk
polycubectl b1 ports tob2 trunk allowed add 14
polycubectl b1 ports tob2 trunk allowed add 10
polycubectl b1 ports add tob3 mode=trunk
polycubectl b1 ports tob3 trunk allowed add 14
polycubectl b1 ports tob3 trunk allowed add 10
polycubectl b1 stp 14 set priority=10
polycubectl b1 stp 10 set priority=1

polycubectl b2 ports add tov2 peer=veth2
polycubectl b2 ports tov2 access set vlanid=14
polycubectl b2 ports add tob1 mode=trunk
polycubectl b2 ports tob1 trunk allowed add 14
polycubectl b2 ports tob1 trunk allowed add 10
polycubectl b2 ports add tob3 mode=trunk
polycubectl b2 ports tob3 trunk allowed add 14
polycubectl b2 ports tob3 trunk allowed add 10
polycubectl b2 stp 14 set priority=100
polycubectl b2 stp 10 set priority=10

polycubectl b3 ports add tov3 peer=veth3
polycubectl b3 ports tov3 access set vlanid=10
polycubectl b3 ports add tob1 mode=trunk
polycubectl b3 ports tob1 trunk allowed add 14
polycubectl b3 ports tob1 trunk allowed add 10
polycubectl b3 ports add tob2 mode=trunk
polycubectl b3 ports tob2 trunk allowed add 14
polycubectl b3 ports tob2 trunk allowed add 10
polycubectl b3 stp 14 set priority=1
polycubectl b3 stp 10 set priority=100

polycubectl connect b1:tob2 b2:tob1
polycubectl connect b1:tob3 b3:tob1
polycubectl connect b2:tob3 b3:tob2

echo "sleeping for convergence.."
sleep $CONVERGENCE_TIME

echo "testing ports.."
#b1
var=$(polycubectl b1 ports tob2 stp 14 show state)
verifyForwarding $var
var=$(polycubectl b1 ports tob2 stp 10 show state)
verifyForwarding $var

var=$(polycubectl b1 ports tob3 stp 14 show state)
verifyForwarding $var
var=$(polycubectl b1 ports tob3 stp 10 show state)
verifyForwarding $var

#b2
var=$(polycubectl b2 ports tob1 stp 14 show state)
verifyBlocking $var
var=$(polycubectl b2 ports tob1 stp 10 show state)
verifyForwarding $var

var=$(polycubectl b2 ports tob3 stp 14 show state)
verifyForwarding $var
var=$(polycubectl b2 ports tob3 stp 10 show state)
verifyForwarding $var

#b3
var=$(polycubectl b3 ports tob1 stp 14 show state)
verifyForwarding $var
var=$(polycubectl b3 ports tob1 stp 10 show state)
verifyForwarding $var

var=$(polycubectl b3 ports tob2 stp 14 show state)
verifyForwarding $var
var=$(polycubectl b3 ports tob2 stp 10 show state)
verifyBlocking $var

echo "testing connectivity.."
# these should arrive
sudo ip netns exec ns1 ping 10.0.0.2 -c 1 -W 1 &>/dev/null
sudo ip netns exec ns2 ping 10.0.0.1 -c 1 -W 1 &>/dev/null

# these should not arrive
test_fail sudo ip netns exec ns1 ping 10.0.0.3 -c 1 -W 1 &>/dev/null
test_fail sudo ip netns exec ns2 ping 10.0.0.3 -c 1 -W 1 &>/dev/null
test_fail sudo ip netns exec ns3 ping 10.0.0.1 -c 1 -W 1 &>/dev/null
test_fail sudo ip netns exec ns3 ping 10.0.0.2 -c 1 -W 1 &>/dev/null

echo "changing topology.."
polycubectl b1 ports tov1 access set vlanid=1
polycubectl b2 ports tov2 access set vlanid=10

echo "sleeping for convergence.."
sleep $CONVERGENCE_TIME

echo "testing connectivity.."
# these should arrive
sudo ip netns exec ns2 ping 10.0.0.3 -c 1 -W 1 &>/dev/null
sudo ip netns exec ns3 ping 10.0.0.2 -c 1 -W 1 &>/dev/null

# these should not arrive
test_fail sudo ip netns exec ns1 ping 10.0.0.2 -c 1 -W 1 &>/dev/null
test_fail sudo ip netns exec ns1 ping 10.0.0.3 -c 1 -W 1 &>/dev/null
test_fail sudo ip netns exec ns2 ping 10.0.0.1 -c 1 -W 1 &>/dev/null
test_fail sudo ip netns exec ns3 ping 10.0.0.1 -c 1 -W 1 &>/dev/null

echo "end."
