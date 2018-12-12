#! /bin/bash

set -x

for i in `seq 1 6`;
do
	sudo ip netns del ns${i} > /dev/null 2>&1	# remove namespace if already present
	sudo ip link del veth${i} > /dev/null 2>&1

	sudo ip netns add ns${i}
	sudo ip link add veth${i}_ type veth peer name veth${i}
	sudo ip link set veth${i}_ netns ns${i}
	sudo ip netns exec ns${i} ip link set dev veth${i}_ up
	sudo ip link set dev veth${i} up
	sudo ip netns exec ns${i} ifconfig veth${i}_ 10.0.0.${i}/24
done

polycubectl pbforwarder add pbfw1
polycubectl pbforwarder pbfw1 ports add veth1
polycubectl pbforwarder pbfw1 ports add veth2
polycubectl pbforwarder pbfw1 ports add veth3
polycubectl pbforwarder pbfw1 ports add veth4
polycubectl pbforwarder pbfw1 ports add veth5
polycubectl pbforwarder pbfw1 ports add veth6

polycubectl pbforwarder pbfw1 ports veth1 set peer="veth1"
polycubectl pbforwarder pbfw1 ports veth2 set peer="veth2"
polycubectl pbforwarder pbfw1 ports veth3 set peer="veth3"
polycubectl pbforwarder pbfw1 ports veth4 set peer="veth4"
polycubectl pbforwarder pbfw1 ports veth5 set peer="veth5"
polycubectl pbforwarder pbfw1 ports veth6 set peer="veth6"

polycubectl pbforwarder pbfw1 rules add 0 inport=veth1 action=FORWARD outport=veth2
polycubectl pbforwarder pbfw1 rules add 1 inport=veth2 action=FORWARD outport=veth1

polycubectl pbforwarder pbfw1 rules add 2 inport=veth3 action=FORWARD outport=veth4
polycubectl pbforwarder pbfw1 rules add 3 inport=veth4 action=FORWARD outport=veth3

polycubectl pbforwarder pbfw1 rules add 4 inport=veth5 action=FORWARD outport=veth6
polycubectl pbforwarder pbfw1 rules add 5 inport=veth6 action=FORWARD outport=veth5
