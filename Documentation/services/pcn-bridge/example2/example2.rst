Example 2 - VLAN
=================

In this example we will test the VLAN support.
We will configure two bridges, and four network namespaces connected to them.

::

            veth1                                            veth3
         10.0.0.1/24                                      10.0.0.3/24
              |                                                |
              |                                                |
  VLAN 1 -->  |                                                | <-- VLAN 1
         +----------+                                     +----------+
         |   br1    |-------------------------------------|   br2    |
         |  (cube)  |    ^                           ^    |  (cube)  |
         +---------.+    |                           |    +----------+
  VLAN 2 -->  |       TRUNK mode                  TRUNK mode   | <-- VLAN 2
              |       allowed 1,2                 allowed 1,2  |
              |                                                |
            veth2                                            veth4
         10.0.0.2/24                                      10.0.0.4/24

The following code configures the network namespaces and virtual network interfaces to be used.

::

    # copy and paste in your terminal

    # namespace ns1 -> veth1 10.0.0.1/24
    # namespace ns2 -> veth2 10.0.0.2/24
    # namespace ns3 -> veth3 10.0.0.3/24
    # namespace ns4 -> veth4 10.0.0.4/24

    for i in `seq 1 4`;
    do
        sudo ip netns del ns${i} > /dev/null 2>&1 # remove ns if already existed
        sudo ip link del veth${i} > /dev/null 2>&1

        sudo ip netns add ns${i}
        sudo ip link add veth${i}_ type veth peer name veth${i}
        sudo ip link set veth${i}_ netns ns${i}
        sudo ip netns exec ns${i} ip link set dev veth${i}_ up
        sudo ip link set dev veth${i} up
        sudo ip netns exec ns${i} ifconfig veth${i}_ 10.0.0.${i}/24
    done


Create bridge instances, and connect virtual interfaces to them

::

    # create instances
    polycubectl bridge add br1
    polycubectl bridge add br2

    # create ports on br1 
    polycubectl br1 ports add toveth1 peer=veth1
    polycubectl br1 ports add toveth2 peer=veth2
    polycubectl br1 ports add tobr2 mode=trunk

    # create ports on br2
    polycubectl br2 ports add toveth3 peer=veth3
    polycubectl br2 ports add toveth4 peer=veth4
    polycubectl br2 ports add tobr1 mode=trunk

    # connect the two bridges
    polycubectl connect br1:tobr2 br2:tobr1
	  
Configure VLANs

::

    # By default, ports are configured in access mode, with VLAN 1
    # Instead, ports in trunk mode have VLAN 1 allowed by default 
    # (and that is also the native vlan)

    # br1
    polycubectl br1 ports toveth2 access set vlanid=2
    polycubectl br1 ports tobr2 trunk allowed add 2

    # br2 
    polycubectl br2 ports toveth4 access set vlanid=2
    polycubectl br2 ports tobr1 trunk allowed add 2

Ping between namespaces

::

    # ping ns3 from ns1
    sudo ip netns exec ns1 ping 10.0.0.3 # ok

    # ping ns4 from ns2
    sudo ip netns exec ns2 ping 10.0.0.4 # ok

    # ping ns4 from ns1
    sudo ip netns exec ns1 ping 10.0.0.4 # packet discarded by br2: not the same VLAN!

Delete bridges

::

    polycubectl br1 del
    polycubectl br2 del

