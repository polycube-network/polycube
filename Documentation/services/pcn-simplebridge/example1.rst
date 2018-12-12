Example 1
=========

In this example two network namespaces are connected together by a simple bridge instance.


The following code configures the network namespaces and virtual network interfaces to be used.

::

    # copy and paste in your terminal

    # namespace ns1 -> veth1 10.0.0.1/24
    # namespace ns2 -> veth2 10.0.0.2/24

    for i in `seq 1 2`;
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


Create a simple bridge instance, add and connects ports to virtual interfaces

::

    # create instance
    polycubectl simplebridge add br0

    # add and connect port to veth1
    polycubectl br0 ports add toveth1 peer=veth1

    # add and connect port to veth2
    polycubectl br0 ports add toveth2 peer=veth2


Ping between namespaces

::

    # ping ns1 from ns2
    sudo ip netns exec ns2 ping 10.0.0.1

Print whole ``br0`` status

::

    polycubectl br0 show


Delete ports

::

    polycubectl br0 ports del toveth1
    polycubectl br0 ports del toveth2

Remove ``br0``

::

    polyubectl del br0

