Tutorial 3: creating a service chain with bridges and routers
=============================================================

This tutorial shows how to create a complex service by means of a topology that includes two routers, two bridges and five virtual interfaces (simulating five hosts).
Each host is a part of a different network and it sends packets to its default gateway, which forwards them to the proper interface based on the (static) routing table.
In this tutorial, the interfaces of the router must have one or more secondary IP addresses to allow hosts belonging to different IP networks to be connected on the same interface of the router.

In addition, this tutorial shows also some more advanced commands of the Router service and how to connect cubes together.

For more details about the features of the Router service, please refer to its :doc:`description <../../services/pcn-router/router>`.


::

     veth1                                                            veth3
    (netdev)                                                         (netdev)
  10.0.1.1/24                                                       10.0.3.1/24
       |                           10.1.0.2/30                          |
       |                                   |                            |
       |                                   |                            |
  +----------+        +----------+         v   +----------+        +----------+
  |   br1    |--------|    r1    |-------------|    r2    |--------|   br2    |---------veth5
  |  (cube)  |     ^  |  (cube)  |   ^         |  (cube)  |  ^     |  (cube)  |        (netdev)
  +---------.+     |  +----------+   |         +----------+  |     +----------+      10.0.5.1/24
       |           |                 |                       |          |
       |          10.0.1.254/24    10.1.0.1/30     10.0.3.254/24        |
       |          10.0.2.254/24                    10.0.4.254/24        |
     veth2                                         10.0.5.254/24       veth4
    (netdev)                                                         (netdev)
  10.0.2.1/24                                                      10.0.4.1/24


Set up namespaces
-----------------

::

    # Namespace ns1
    #  veth1 10.0.1.1/24
    #  default gateway 10.0.1.254
    # Namespace ns2
    #  veth2 10.0.2.1/24
    #  default gateway 10.0.2.254
    # Namespace ns3
    #  veth3 10.0.3.1/24
    #  default gateway 10.0.3.254
    # Namespace ns4
    #  veth4 10.0.4.1/24
    #  default gateway 10.0.4.254
    # Namespace ns5
    #  veth5 10.0.5.1/24
    #  default gateway 10.0.5.254

    for i in `seq 1 5`;
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


Step 1: Create cubes, routers r1 and r2, bridges br1 and br2
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    polycubectl router add r1
    polycubectl router add r2
    polycubectl simplebridge add br1
    polycubectl simplebridge add br2

Step 2: Add and connect ports on bridges
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    # Connect veth1 and veth2 to br1
    polycubectl br1 ports add to_veth1 peer=veth1
    polycubectl br1 ports add to_veth2 peer=veth2

    # Connect veth3, veth4 and veth5 to br2
    polycubectl br2 ports add to_veth3 peer=veth3
    polycubectl br2 ports add to_veth4 peer=veth4
    polycubectl br2 ports add to_veth5 peer=veth5

Step 3: Add and connect ports on routers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Connect** ``r1`` **to** ``br1``

::

    # Create new port on r1 and set the IP parameters
    polycubectl r1 ports add to_br1 ip=10.0.1.254/24
    # Create a bridge interface on br1
    polycubectl br1 ports add to_r1
    # Connect both ports
    polycubectl connect br1:to_r1 r1:to_br1


**Create secondary address**

In order to permit more networks on the same router interface we need to add a secondary address to the router interface which will be the default gateway for a new network. The router automatically adds a local route as before.

::

    # Add a secondary address on r1 interface `to_br1`
    polycubectl r1 ports to_br1 secondaryip add 10.0.2.254/24


**Connect** ``r2`` **to** ``br2``

::

    # Create new port on r2 and set the IP parameters
    polycubectl r2 ports add to_br2 ip=10.0.3.254/24

    # Create a bridge interface on br2
    polycubectl br2 ports add to_r2
    # Connect both ports
    polycubectl connect br2:to_r2 r2:to_br2

**Create secondary address**

In the router ``r2`` we have three different networks, so we need to add two secondary addresses

::

    # Add the secondary addresses on r2 interface `to_br2`
    polycubectl r2 ports to_br2 secondaryip add 10.0.4.254/24
    polycubectl r2 ports to_br2 secondaryip add 10.0.5.254/24

**Connect the routers**

We need to create a point-to-point link between the routers to connect them. To do this, we will use a /30 network

::

    # Create new port on r1 and r2 and set the IP parameters
    polycubectl r1 ports add to_r2 ip=10.1.0.1/30
    polycubectl r2 ports add to_r1 ip=10.1.0.2/30

    # Connects the routers
    polycubectl connect r1:to_r2 r2:to_r1

Step 4: Fill up routing tables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Add static entries in the routing table of router `r1`**

We need to tell the router `r1` which are the networks reachable through `r2`

::

    polycubectl r1 route add 10.0.3.0/24 10.1.0.2
    polycubectl r1 route add 10.0.4.0/24 10.1.0.2
    polycubectl r1 route add 10.0.5.0/24 10.1.0.2


**Add static entries in the routing table of router `r2`**

We need to do the same on the router `r2`

::

    polycubectl r2 route add 10.0.1.0/24 10.1.0.1
    polycubectl r2 route add 10.0.2.0/24 10.1.0.1


**Show the routing tables of the routers**

We can see all the entries of a routing table in a router using the ``show`` command in the ``polycubectl``

::

    polycubectl r1 route show
    polycubectl r2 route show

Step 5: Test the connectivity between the namespaces and the router
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can test the connectivity between each host (i.e., veth in the namespace) and the routers, on all their interfaces, using ``ping``:

::

    # Ping interfaces from ns1
    sudo ip netns exec ns1 ping 10.0.1.254 -c 1
    sudo ip netns exec ns1 ping 10.0.2.254 -c 1
    sudo ip netns exec ns1 ping 10.1.0.1 -c 1
    sudo ip netns exec ns1 ping 10.1.0.2 -c 1

    # Ping interfaces from ns2
    sudo ip netns exec ns2 ping 10.0.1.254 -c 1
    sudo ip netns exec ns2 ping 10.0.2.254 -c 1
    sudo ip netns exec ns2 ping 10.1.0.1 -c 1
    sudo ip netns exec ns2 ping 10.1.0.2 -c 1

    # Ping interfaces from ns3
    sudo ip netns exec ns3 ping 10.0.1.254 -c 1
    sudo ip netns exec ns3 ping 10.0.2.254 -c 1
    sudo ip netns exec ns3 ping 10.1.0.1 -c 1
    sudo ip netns exec ns3 ping 10.1.0.2 -c 1

    # Ping interfaces from ns4
    sudo ip netns exec ns4 ping 10.0.1.254 -c 1
    sudo ip netns exec ns4 ping 10.0.2.254 -c 1
    sudo ip netns exec ns4 ping 10.1.0.1 -c 1
    sudo ip netns exec ns4 ping 10.1.0.2 -c 1

    # Ping interfaces from ns5
    sudo ip netns exec ns5 ping 10.0.1.254 -c 1
    sudo ip netns exec ns5 ping 10.0.2.254 -c 1
    sudo ip netns exec ns5 ping 10.1.0.1 -c 1
    sudo ip netns exec ns5 ping 10.1.0.2 -c 1

You can ping the secondary addresses too.

Step 5: Test the connectivity between different namespaces
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Now you can test the connectivity between all the different namespaces:

::

    # Ping ns2 from ns1
    sudo ip netns exec ns1 ping 10.0.2.1 -c 1

    # Ping ns3 from ns1
    sudo ip netns exec ns1 ping 10.0.3.1 -c 1

    # Ping ns4 from ns1
    sudo ip netns exec ns1 ping 10.0.4.1 -c 1

    # Ping ns5 from ns1
    sudo ip netns exec ns1 ping 10.0.5.1 -c 1


and so on.

You can do a complete ping test running `ping.sh <ping.sh>`_.

Step 6: Test the `TIME_TO_LIVE EXCEEDED` message
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When a router receives a packet with the time-to-live equal or less than 1, it discards the packet and sends to the sender an ICMP TIME_TO_LIVE EXCEEDED message.
To test this function you can simply do a ping to an host setting the ttl option to 1.

::

    # Ping ns2 from ns1 with TTL=1
    sudo ip netns exec ns1 ping 10.0.2.1 -c 1 -t 1
