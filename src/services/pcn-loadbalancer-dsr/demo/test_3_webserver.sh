#!/bin/bash

source "./../helpers.bash"

FLAG="TC"

function cleanup {
  polycubectl bridge del br1
  polycubectl lbdsr del lb1
  delete_veth 4
  delete_link 2
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

create_veth 4
create_link 2

polycubectl bridge add br1 type=$FLAG
polycubectl lbdsr add lb1 type=$FLAG

// backend servers
bridge_add_port br1 veth1
bridge_add_port br1 veth2
bridge_add_port br1 veth3

// clients
bridge_add_port br1 veth4

// lbdsr ports
bridge_add_port br1 link12
bridge_add_port br1 link22

// config lb
polycubectl lbdsr lb1 ports add link11
polycubectl lbdsr lb1 ports add link21
polycubectl lbdsr lb1 ports link11 set peer=link11
polycubectl lbdsr lb1 ports link21 set peer=link21

// configure webservers not to responde to arp requests
mac1=$(sudo ip netns exec ns1 ifconfig | grep veth1_ | awk '{print $5}')
polycubectl lbdsr lb1 backend pool add 1 mac=$mac1
sudo ip netns exec ns1 sudo ifconfig lo 10.0.0.100 netmask 255.255.255.255 up
sudo ip netns exec ns1 sudo sysctl -w net.ipv4.conf.all.arp_ignore=1
sudo ip netns exec ns1 sudo sysctl -w net.ipv4.conf.all.arp_announce=2

mac2=$(sudo ip netns exec ns2 ifconfig | grep veth2_ | awk '{print $5}')
polycubectl lbdsr lb1 backend pool add 2 mac=$mac2
sudo ip netns exec ns2 sudo ifconfig lo 10.0.0.100 netmask 255.255.255.255 up
sudo ip netns exec ns2 sudo sysctl -w net.ipv4.conf.all.arp_ignore=1
sudo ip netns exec ns2 sudo sysctl -w net.ipv4.conf.all.arp_announce=2

mac3=$(sudo ip netns exec ns3 ifconfig | grep veth3_ | awk '{print $5}')
polycubectl lbdsr lb1 backend pool add 3 mac=$mac3
sudo ip netns exec ns3 sudo ifconfig lo 10.0.0.100 netmask 255.255.255.255 up
sudo ip netns exec ns3 sudo sysctl -w net.ipv4.conf.all.arp_ignore=1
sudo ip netns exec ns3 sudo sysctl -w net.ipv4.conf.all.arp_announce=2

sleep 10000
