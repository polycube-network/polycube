Tutorial 1: the simplebridge service
====================================

This tutorial shows how to create a simple topology with a bridge and a couple of virtual interfaces (`netdev`), belonging to different namespaces, attached to it.


::

                 +----------+
     veth1 ------|   br0    |------ veth2
    (netdev)     |  (cube)  |      (netdev)
                 +----------+


Set up namespaces
-----------------
The following code configures the network namespaces.

::

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



Deploy topology
---------------

Step 1: Create the bridge `br1`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    polycubectl simplebridge add br1


If everything goes fine, you shouldn't see any error message on the terminal.

Step 2: Add and connect ports
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There are different ways to connect ports to netdevs, this step shows how to use the ``connect`` command and how to set the ``peer`` property.


**Connect** ``veth1`` **to** ``br1``

::

    # way 1
    # create new port on br1
    polycubectl br1 ports add toveth1

    # connect port to netdev
    polycubectl connect br1:toveth1 veth1


**Connect** ``veth2`` **to** ``br1``

::

    # way 2
    # create a port and connect to a netdev using a single command
    polycubectl br1 ports add toveth2 peer=veth2


Show the current status of br1 to check that it is configured as desired (ports are present and its ``peer`` are the veth interfaces)

::

    polycubectl br1 show
    name: br1
    uuid: ed3e477a-4138-4638-9fe0-7cbd31f1d1fb
    type: type_tc
    loglevel: info
    fdb:
     aging-time: 300

    ports:
     name     uuid                                  status  peer   mac
     toveth2  a13d533f-6aeb-49d0-a05c-a048dca31473  up      veth2  1a:68:52:5e:0b:05
     toveth1  d2f1b70b-ff41-4b08-b711-f1b1e9c84786  up      veth1  12:0d:1f:02:d6:3f



Step 3: Test the connectivity
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Now you can test the connectivity between namespaces using `ping`:

::

    # ping ns2 from ns1
    sudo ip netns exec ns1 ping 10.0.0.2 -c 1

    # ping ns1 from ns2
    sudo ip netns exec ns2 ping 10.0.0.1 -c 1

After you perform the ping, you can show once again the br1 status, notice that there are new entries in the filtering database.

::

    polycubectl br1 show
    name: br1
    uuid: ed3e477a-4138-4638-9fe0-7cbd31f1d1fb
    type: type_tc
    loglevel: info
    fdb:
     aging-time: 300

     entry:
      address            port     age
      46:2b:1f:a3:d1:81  toveth2  4
      2a:d4:9a:9a:8b:b5  toveth1  4

    ports:
     name     uuid                                  status  peer   mac
     toveth2  a13d533f-6aeb-49d0-a05c-a048dca31473  up      veth2  1a:68:52:5e:0b:05
     toveth1  d2f1b70b-ff41-4b08-b711-f1b1e9c84786  up      veth1  12:0d:1f:02:d6:3f


Step 4: Remove ``br1``
^^^^^^^^^^^^^^^^^^^^^^

::

    # first remove the ports (this step is optional, removing the bridge will do it)
    polycubectl br1 del ports toveth1
    polycubectl br1 del ports toveth2
    # then destroy the bridge
    polycubectl del br1
