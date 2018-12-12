#!/usr/bin/env bash

function del_pbforwarders {
  for i in `seq 1 $1`;
  do
    polycubectl pbforwarder del pbf$i
  done
}

function del_veths {
  for i in `seq 1 $1`;
  do
    sudo ip link del veth${i}
    sudo ip netns del ns${i}
  done
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
    sudo ip netns exec ns${i} ip -s neigh flush all
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

function add_pbforwarders {
  for i in `seq 1 $1`;
  do
    polycubectl pbforwarder add pbf$i
  done
}

function pbforwarder_add_port {
  polycubectl pbforwarder $1 ports add $2
  polycubectl pbforwarder $1 ports $2 set peer=$2
}

function pbforwarder_add_rules_l2 {
  for i in `seq $1 $2`;
  do
    number=$((i%100));
    if [[ ${#number} -lt 2 ]] ; then
      number="00${number}"
      number="${number: -2}"
    fi
    polycubectl pbforwarder $3 rules add $i src_mac=00:00:00:00:11:$number action=DROP
  done
}

function pbforwarder_add_rules_l3 {
  for i in `seq $1 $2`;
  do
    number=$((i%256));
    polycubectl pbforwarder $3 rules add $i src_ip=10.9.8.$number action=DROP
  done
}

function pbforwarder_add_rules_l4 {
  for i in `seq $1 $2`;
  do
    polycubectl pbforwarder $3 rules add $i l4_proto=TCP action=DROP
  done
}
