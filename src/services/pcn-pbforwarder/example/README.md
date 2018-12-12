# pcn-pbforwarder example: creating a service chain between network namespaces

This examples shows how to create a service chain through different Linux namespaces, where the forwarding is handled a *single* instance of the `pcn-pbforwarder` service, which exports *six* ports and connect them properly.

In a more complex example, the `pcn-pbforwarder` could be replaced by other services, hence creating a true service chain encompassing multiple services (e.g., bridge, router, etc).


```
client      service1          service2         server
 ns1           ns2               ns3            ns4
  ^          ^    ^            ^    ^            ^
  |          |    |            |    |            |
veth1      veth2 veth3      veth4  veth5       veth6
  |          |    |            |    |            |  
+----------------------------------------------------+
| | FORWARD  |    |  FORWARD   |    |  FORWARD   |   |
| +---<<>>---+    +----<<>>----+    +----<<>>----+   |
|                                                    |
|               policy based forwarder               |
|                      pbfw1                         |
|                   (cube)                       |
+----------------------------------------------------+
```

## Run polycubed
Launch the main `polycubed` daemon:
```
# Run polycubed
sudo polycubed
```

## Set up virtual interfaces and namespaces
Set up linux namespaces and links:

```
	sudo ip netns add ns1
	sudo ip link add veth1_ type veth peer name veth1
	sudo ip link set veth1_ netns ns1
	sudo ip netns exec ns1 ip link set dev veth1_ up
	sudo ip link set dev veth1 up
	sudo ip netns exec ns1 ifconfig veth1_ 10.0.0.1/24

	sudo ip netns add ns2
	sudo ip link add veth2_ type veth peer name veth2
	sudo ip link set veth2_ netns ns2
	sudo ip netns exec ns2 ip link set dev veth2_ up
	sudo ip link set dev veth2 up
	sudo ip netns exec ns2 ifconfig veth2_ 10.0.0.2/24

	sudo ip netns add ns3
	sudo ip link add veth3_ type veth peer name veth3
	sudo ip link set veth3_ netns ns3
	sudo ip netns exec ns3 ip link set dev veth3_ up
	sudo ip link set dev veth3 up
	sudo ip netns exec ns3 ifconfig veth3_ 10.0.0.3/24

	sudo ip netns add ns4
	sudo ip link add veth4_ type veth peer name veth4
	sudo ip link set veth4_ netns ns4
	sudo ip netns exec ns4 ip link set dev veth4_ up
	sudo ip link set dev veth4 up
	sudo ip netns exec ns4 ifconfig veth4_ 10.0.0.4/24

	sudo ip netns add ns5
	sudo ip link add veth5_ type veth peer name veth5
	sudo ip link set veth5_ netns ns5
	sudo ip netns exec ns5 ip link set dev veth5_ up
	sudo ip link set dev veth5 up
	sudo ip netns exec ns5 ifconfig veth5_ 10.0.0.5/24

	sudo ip netns add ns6
	sudo ip link add veth6_ type veth peer name veth6
	sudo ip link set veth6_ netns ns6
	sudo ip netns exec ns6 ip link set dev veth6_ up
	sudo ip link set dev veth6 up
	sudo ip netns exec ns6 ifconfig veth6_ 10.0.0.6/24
```

## Create a Policy Based Forwarder

```
polycubectl pbforwarder add pbfw1
```

## Connect ports to pbfw1

```
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

```

## Setup rules to redirect traffic to a chain

```
polycubectl pbforwarder pbfw1 rules add 0 inport=veth1 action=FORWARD outport=veth2
polycubectl pbforwarder pbfw1 rules add 1 inport=veth2 action=FORWARD outport=veth1

polycubectl pbforwarder pbfw1 rules add 2 inport=veth3 action=FORWARD outport=veth4
polycubectl pbforwarder pbfw1 rules add 3 inport=veth4 action=FORWARD outport=veth3

polycubectl pbforwarder pbfw1 rules add 4 inport=veth5 action=FORWARD outport=veth6
polycubectl pbforwarder pbfw1 rules add 5 inport=veth6 action=FORWARD outport=veth5  

```

## Automatic set up
In order to avoid to manually type all the above commands, an automatic script that creates all the setup is available as [example_setup.sh](example_setup.sh).

You can launch the above script; then move into one of the namespaces and see if the IP traffic is correctly forwarded (e.g., `sudo ip netns exec ns1 ping 10.0.0.6`).
