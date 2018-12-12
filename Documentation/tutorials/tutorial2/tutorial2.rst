Tutorial 2: the router service
================================

This tutorial shows the basic commands of the ``router`` service.
It creates a network topology with one router with three virtual interfaces (simulating three hosts) connected to it.
Each host sends packets to its default gateway (i.e., the router interface connected to it), which forwards them to the proper interface based on the (static) routing table.

An introduction to the use of the ``polycubectl`` is also presented.

For more details about the features of the Router service, please refer to its :doc:`description <../../services/pcn-router/router>`.

::

                         +----------+
             veth1 ------|    r1    |------- veth2
            (netdev)     |  (cube)  |       (netdev)
                         +----------+
                               |
                               |
                             veth3
                            (netdev)

Set up namespaces
-----------------

::

    # namespace ns1
    #  veth1 10.0.1.1/24
    #  default gateway 10.0.1.254
    # namespace ns2
    #  veth2 10.0.2.1/24
    #  default gateway 10.0.2.254
    # namespace ns3
    #  veth3 10.0.3.1/24
    #  default gateway 10.0.3.254

    for i in `seq 1 3`;
    do
        sudo ip netns del ns${i} > /dev/null 2>&1 # remove ns if already existed
        sudo ip link del veth${i} > /dev/null 2>&1

        sudo ip netns add ns${i}
        sudo ip link add veth${i}_ type veth peer name veth${i}
        sudo ip link set veth${i}_ netns ns${i}
        sudo ip netns exec ns${i} ip link set dev veth${i}_ up
        sudo ip link set dev veth${i} up
        sudo ip netns exec ns${i} ifconfig veth${i}_ 10.0.${i}.1/24
        sudo ip netns exec ns${i} route add default gw 10.0.${i}.254 veth${i}_
    done




Deploy topology
---------------

Step 1: Create the router ``r1``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    polycubectl router add r1 loglevel=INFO


Step 2: Add and connect ports
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Before trying to add a port, let's use the ``polycubectl`` help to get the info about what are the parameters supported by the port.

::

    polycubectl r1 ports add ?

    Keyword             Type     Description
     <name>              string   Port Name

     Other parameters:
     peer=value          string   Peer name, such as a network interfaces (e.g., 'veth0') or another cube (e.g., 'br1:port2')
     ip=value            string   IP address of the port
     netmask=value       string   Netmask of the port
     mac=value           string   MAC address of the port
    Example:
     polycubectl r1 ports add port1 peer=r0:port1 ip=207.46.130.1 netmask=255.255.255.0 mac=B3:23:45:F5:3A


The output indicates that a port name is expected ``Keyword``, and following it the ``peer``, ``ip``, ``netmask`` and ``mac`` are supported parameters.


**Connect** ``veth1`` **to** ``r1``


::

    # create new port on r1 and set the IP parameters
    polycubectl r1 ports add to_veth1 ip=10.0.1.254 netmask=255.255.255.0
    # connect port to netdev
    polycubectl connect r1:to_veth1 veth1

The router automatically adds a new local entry in the routing table.

::

    polycubectl r1 show route
    interface  netmask        network   nexthop  pathcost
    to_veth1   255.255.255.0  10.0.1.0  local    0



**Connect** ``veth2`` **to** ``r1``

::

    # create new port on r1 and set the IP parameters
    polycubectl r1 ports add to_veth2 ip=10.0.2.254 netmask=255.255.255.0

    # connect router port to netdev interface
    polycubectl r1 ports to_veth2 set peer=veth2
    # You could also used the 'connect' command:
    # polycubectl connect r1:to_veth2 veth2


**Connect** ``veth3`` **to** ``r1``

::

    # create new port on r1 and set the IP parameters
    # notice that in this case 'peer' is also set so the port is also connected to the netdev
    polycubectl r1 ports add to_veth3 ip=10.0.3.254 netmask=255.255.255.0 peer=veth3


Step 3: Check configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can use the show command to print the whole configuration of the router.
You should see an output similar to the next one, where ports are ``up`` and have ``peer`` set to the right interface.

::

    polycubectl r1 show
    name: r1
    uuid: b8fd2a02-064e-461e-98d4-d9b7fba384a2
    type: type_tc
    loglevel: info

    ports:
     name      uuid                                  status  peer   ip          netmask        mac
     to_veth3  c51bb0ed-9e6f-44ed-a096-b13bc1011331  up      veth3  10.0.3.254  255.255.255.0  72:59:a8:c2:c2:44
     to_veth2  48f8d130-aa32-4354-a1b5-105df9a8ad7b  up      veth2  10.0.2.254  255.255.255.0  d6:42:7f:65:b4:40
     to_veth1  46c685b9-4c80-4466-9d81-985598a07444  up      veth1  10.0.1.254  255.255.255.0  52:f0:5f:2c:a5:a7

    route:
     network   netmask        nexthop  interface  pathcost
     10.0.1.0  255.255.255.0  local    to_veth1   0
     10.0.2.0  255.255.255.0  local    to_veth2   0
     10.0.3.0  255.255.255.0  local    to_veth3   0


Step 4: Test the connectivity between the namespaces and the router
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can test the connectivity between each host (i.e., veth in the namespace) and the router, on all its interfaces, using `ping`:

::

    # Ping interfaces from ns1
    sudo ip netns exec ns1 ping 10.0.1.254 -c 1
    sudo ip netns exec ns1 ping 10.0.2.254 -c 1
    sudo ip netns exec ns1 ping 10.0.3.254 -c 1

    # Ping interfaces from ns2
    sudo ip netns exec ns2 ping 10.0.1.254 -c 1
    sudo ip netns exec ns2 ping 10.0.2.254 -c 1
    sudo ip netns exec ns2 ping 10.0.3.254 -c 1

    # Ping interfaces from ns3
    sudo ip netns exec ns3 ping 10.0.1.254 -c 1
    sudo ip netns exec ns3 ping 10.0.2.254 -c 1
    sudo ip netns exec ns3 ping 10.0.3.254 -c 1

Step 5: Test the connectivity between different namespaces
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Now you can test the connectivity between all the different namespaces using `ping`:

::

    # Ping ns2 from ns1
    sudo ip netns exec ns1 ping 10.0.2.1 -c 1

    # Ping ns3 from ns1
    sudo ip netns exec ns1 ping 10.0.3.1 -c 1

    # Ping ns1 from ns2
    sudo ip netns exec ns2 ping 10.0.1.1 -c 1

    # Ping ns3 from ns2
    sudo ip netns exec ns2 ping 10.0.3.1 -c 1

    # Ping ns1 from ns3
    sudo ip netns exec ns3 ping 10.0.1.1 -c 1

    # Ping ns2 from ns3
    sudo ip netns exec ns3 ping 10.0.2.1 -c 1


Step 6: Remove the router instance
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    # delete r1 and its ports
    polycubectl del r1
