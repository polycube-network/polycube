#!/bin/bash
set -x

#Deleting firewall
polycubectl firewall del fw

#Deleting standard cube
polycubectl simplebridge del br

#Deleting namespaces
for i in `seq 1 2`;do
  	sudo ip link del veth${i}
  	sudo ip netns del ns${i}
done