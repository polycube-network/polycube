Example 3 - Spanning Tree
=========================

In this example we will test the Spanning Tree configuration.
We will have three bridges connected each other in a triangle.

::

    priority: 32768 (default)                            priority: 28672
              |                                                 |
         +----------+                                     +----------+
         |   br1    |-------------------------------------|   br2    |
         |  (cube)  |                                     |  (cube)  |
         +---------.+                                     +----------+
              |                                                 |
              |                                                 |
              |                 +----------+                    |
              \-----------------|   br3    |--------------------/
                                |  (cube)  |
                                +---------.+
                                     |
                              priority: 24576

In this configuration, we expect that bridge ``br3`` will be the root bridge (lower priority).
Furthermore, according to the STP algorithm, the port that should be blocked is port ``br1:tobr2``.


Create bridge instances and connect them each other

::

    # create instances
    polycubectl bridge add br1
    polycubectl bridge add br2
    polycubectl bridge add br3

    # add ports
    polycubectl br1 ports add tobr2
    polycubectl br1 ports add tobr3
    polycubectl br2 ports add tobr1
    polycubectl br2 ports add tobr3
    polycubectl br3 ports add tobr1
    polycubectl br3 ports add tobr2

    # connect ports
    polycubectl connect br1:tobr2 br2:tobr1
    polycubectl connect br1:tobr3 br3:tobr1
    polycubectl connect br2:tobr3 br3:tobr2

Enable STP in each bridge

::

    polycubectl br1 set stp-enabled=true
    polycubectl br2 set stp-enabled=true
    polycubectl br3 set stp-enabled=true

Change priority of bridges

::

    # In each bridge, STP instance of VLAN 1 is active by default
    # (all the ports are configured by default in access mode with VLAN 1)

    # Default bridge priority: 32768
    polycubectl br2 stp 1 set priority=28672
    polycubectl br3 stp 1 set priority=24576

    # Wait for convergence
    sleep 50

Check ports

::

    # br1
    polycubectl br1 ports tobr2 stp 1 show state # blocking
    polycubectl br1 ports tobr3 stp 1 show state # forwarding

    # br2
    polycubectl br2 ports tobr1 stp 1 show state # forwarding
    polycubectl br2 ports tobr3 stp 1 show state # forwarding

    # br3
    polycubectl br3 ports tobr1 stp 1 show state # forwarding
    polycubectl br3 ports tobr2 stp 1 show state # forwarding

Delete bridges

::

    polycubectl br1 del
    polycubectl br2 del
    polycubectl br3 del
