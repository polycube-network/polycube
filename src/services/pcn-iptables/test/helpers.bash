#!/usr/bin/env bash

ip=8.8.8.8

# uncomment this line to run tests in XDP mode
# TYPE="XDP"

function launch_iptables {
  if [ -z "$TYPE" ]; then
    polycubectl iptables add pcn-iptables loglevel=TRACE
  else
    polycubectl iptables add pcn-iptables type=TYPE_XDP_DRV loglevel=TRACE
  fi
}

function create_veth {
    for i in `seq 1 $1`;
    do
        sudo ip netns add ns${i}
        sudo ip link add veth${i}_ type veth peer name veth${i}
        sudo ip link set veth${i}_ netns ns${i}
        sudo ip netns exec ns${i} ip link set dev veth${i}_ up
        sudo ip link set dev veth${i} up
        sudo ip netns exec ns${i} ifconfig veth${i}_ 10.0.0.${i}/24
    done
}

function create_veth_net {
    for i in `seq 1 $1`;
    do
        sudo ip netns add ns${i}
        sudo ip link add veth${i}_ type veth peer name veth${i}
        sudo ip link set veth${i}_ netns ns${i}
        sudo ip netns exec ns${i} ip link set dev veth${i}_ up
        sudo ip link set dev veth${i} up

        sudo ifconfig veth${i} 10.0.${i}.254/24 up

        sudo ip netns exec ns${i} ifconfig veth${i}_ 10.0.${i}.1/24
        sudo ip netns exec ns${i} sudo ip route add default via 10.0.${i}.254
    done
}

function create_link {
    for i in `seq 1 $1`;
    do
        sudo ip link add link${i}1 type veth peer name link${i}2
        sudo ip link set dev link${i}1 up
        sudo ip link set dev link${i}2 up
    done
}

function delete_veth {
    for i in `seq 1 $1`;
    do
        sudo ip link del veth${i}
        sudo ip netns del ns${i}
    done
}

function delete_link {
    for i in `seq 1 $1`;
    do
        sudo ip link del link${i}1
    done
}

function test_fail {
    set +e
    res=$($@)
    local status=$?
    set -e
    if [ $status -ne 0 ]; then
        return 0
    else
        return 1
    fi
}

function enable_ip_forwarding {
    echo "Enabling ip_forwaridng"
    sudo sysctl -w net.ipv4.ip_forward=1
    echo "/proc/sys/net/ipv4/ip_forward"
    cat /proc/sys/net/ipv4/ip_forward
}