Example 1 - Connectivity
========================

In this example two network namespaces will be connected together by a bridge instance.


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


Create a bridge instance, add and connects ports to virtual interfaces

::

    # create instance
    polycubectl bridge add br1

    # add and connect port to veth1
    polycubectl br1 ports add toveth1 peer=veth1

    # add and connect port to veth2
    polycubectl br1 ports add toveth2 peer=veth2


Ping between namespaces

::

    # ping ns2 from ns1
    sudo ip netns exec ns1 ping 10.0.0.2

Print whole ``br1`` status

::

    polycubectl br1 show


Delete ports

::

    polycubectl br1 ports del toveth1
    polycubectl br1 ports del toveth2

Remove ``br1``

::

    polyubectl del br1

