Transparent Helloworld
======================

This service is oriented to demonstrate to developers how to create a minimal transparent cube, which includes both the dataplane `fast path` and the control/management `slow path`.

The behaviour of this service is very similar as :doc:`Helloworld <services/pcn-helloworld/helloworld>`; however, it attaches to an existing interface (in fact, it is *transparent*) instead of having its own set of interfaces.
From the developer point of view, it inherits from the ``polycube-transparent-base`` datamodel instead of ``polycube-standard-base``.

Transparent Helloworld is a simple service that receives the traffic on a network interface and can either:

- allow packets to pass
- send packets to the slow path
- drop packets

The behavior of this service can be changed by setting the ``ingress-action`` and ``egress-action`` flags, which tell the data plane how packets have to be processed.
In fact, this service can distinguish between *ingress* (i.e., from the network device up to the TCP/IP stack) or *egress* (vice versa) packets, enabling to set different actions for them.


How to use
----------

::

    # we assume a physical interface named 'eth0'
    # is available in the system

    # create a transparent hello world
    polycubectl transparenthelloworld add thw0

    # attach to eth0
    polycubectl attach thw0 eth0

    # you can ping the Internet from your host
    #    e.g., ping 8.8.8.8
    # and see that the ping works properly

    # now let's change action in the ingress-path
    polycubectl thw0 set ingress-action=slowpath

    # now you can see that the ping does no longer work
    #
    # in exchange, polycubed prints a log message each
    # time it receives a new packet in the slow path

    # restore 'pass' behavior
    polycubectl thw0 set ingress-action=pass

    # now the 'ping' works again


