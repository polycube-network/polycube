NAT
===

This service implements a NAT, network address translation working at layer 3. In particular, the service can remap source and destination IP addresses and source and destination ports in IP datagram packet headers while they are transit across a traffic routing device.

Features
--------

- Transparent NAT
- IPv4 support
- Support ICMP, TCP and UDP traffic
- Support for Source NAT, Masquerade, Destination NAT and Port Forwarding

Limitations
-----------

- Unsupported IPv6
- Unsupported ARP: ARP messages are forwarded without any processing
- Incremental external port choice, starting from 1024 each time 65535 is reached

How to use
----------

The NAT is a transparent service, it can be attached to a cube port or to a netdev.
The NAT is intended to be used with a router, it may not work when attached to different services.

Attached to a router
^^^^^^^^^^^^^^^^^^^^

When a NAT instance is attached to a router, the external IP is automatically configured.

::

    # create nat
    polycubectl nat add nat1

    # attach to a router port (it should exists)
    polycubectl attach nat1 r1:port1


Attached to a netdev
^^^^^^^^^^^^^^^^^^^^

::

    # create nat
    polycubectl nat add nat1

    # attach to a router port (it should alraedy exist)
    polycubectl attach nat1 eth0

    # configure external IP manually
    # TODO: there is not an API for it!


Working modes
-------------


Source NAT
^^^^^^^^^^

Source NAT selects a given range of hosts (i.e., the ```internal-net```) to be translated with a given public IP address (i.e., the ```external-ip```).
Each time a new session originated from the internal network is recognized, the NAT creates a new binding also for return packets.

Source NAT enables users to specify exactly the range of addresses belonging to the ```internal-net```, e.g., excluding some hosts from being natted. In addition, it specifies exactly the external IP that has to be used, e.g., in case multiple external IP addresses are available.

Available and mandatory fields for SNAT rules are:
 - ```internal-net```: the internal network to translate (e.g 10.0.0.0/24, 10.0.0.1/32)
 - ```external-ip```: the external IP address (e.g. 1.2.3.4)

Sample rule:

::

    polycubectl nat1 rule snat append internal-net=10.0.0.0/24 external-ip=1.2.3.4


Masquerade
^^^^^^^^^^

NAT masquerading is a simpler form of source NAT: it automatically translates the traffic of all hosts coming from the internal network with the IP address currently present on the external interface.
Hence, it requires a simpler configuration that is usually enough for most cases.

Unlike ```iptables``` in Linux, NAT masquerading in Polycube does not add extra overhead compared to SNAT.
In fact, the ```external-ip``` address is kept in a dedicated table in the data plane and it is automatically updated each time the actual IP address configured on the external virtual interface is changed.
This avoid to check which IP address has to be used each time a new packet is received.

Masquerade can be either enabled or disabled.
When enabled, the source IP address of outgoing packets is set to the external IP of the NAT, and a new source port is assigned.

To enable masquerade:

::

    polycubectl nat1 rule masquerade enable

To disable masquerade:

::

    polycubectl nat1 rule masquerade disable


Destination NAT
^^^^^^^^^^^^^^^

Destination NAT adds a new translation rule for traffic coming from the external network directed to the internal network. It implements what is also called `static` NAT in other contexts.
This features allows external traffic to be forwarded to the internal network without having a corresponding session already established from inside.

The destination NAT forwards all the traffic coming to a specific ```external-ip``` to a given ```internal-ip```, without inspecting other packet fields such as TCP/UDP ports, or protocols (e.g., ICMP).
Obviously, this option requires a dedicated ```external-ip``` for each mapping.

Available and mandatory fields for DNAT rules are:
 - ```internal-ip```: the internal IP address (e.g 10.0.0.1)
 - ```external-ip```: the external IP address (e.g. 1.2.3.4)

Sample rule:

::

    polycubectl nat1 rule dnat append internal-ip=10.0.0.1 external-ip=1.2.3.4

All the packets directed to ```1.2.3.4``` will be sent to ```10.0.0.1```, so use this rule carefully.


Port Forwarding
^^^^^^^^^^^^^^^

Port forwarding implements a more selective form of destination NAT, allowing to specify the exact binding by means of other parameters such as the destination TCP/UDP port. This enables the sharing of the same ```external-ip``` among many ```internal-ip``` hosts, provided that their traffic can be differentiated e.g., by means of other parameters such as the ```external-port``` in use. 

Available and mandatory fields for Port Forwarding rules are:
 - ```internal-ip```: the internal IP address (e.g 10.0.0.1)
 - ```external-ip```: the external IP address (e.g. 1.2.3.4)
 - ```internal-port```: the internal port number (e.g 8080)
 - ```external-port```: the external port number (e.g. 80)
 - ```proto```: the upper layer protocol (e.g. tcp, udp, all). This field is optional: if not specified, all protocols are considered

Sample rule:


::

    polycubectl nat1 rule port-forwarding append \
    external-ip=1.2.3.4 external-port=80 internal-ip=10.0.0.1 \
    internal-port=8080 proto=tcp

This type of rule is especially useful to make a server in the inside network reachable from the outside.


Other NAT operations
--------------------

Deleting rules
^^^^^^^^^^^^^^

It is possible to delete all rules together, all the rules of the same type together, or single rules.

To delete all rules:

::

    polycubectl nat1 rule del

To delete all rules of a type (SNAT in the example):

::

    polycubectl nat1 rule snat del

To delete a single rule (an SNAT rule in the example):

::

    polycubectl nat1 rule snat del RULE_ID

Deleting a rule does not affect ongoing natting sessions: to prevent a deleted rule from being applied, `flush the natting table <Natting-table>`_.


Showing rules
^^^^^^^^^^^^^

It is possible to display all existing rules at the same time, or only a specific type of rule.

To display all rules:

::

    polycubectl nat1 rule show

To display one type of rule (SNAT in the example):

::

    polycubectl nat1 rule snat show


Rule priority
^^^^^^^^^^^^^

Explicit rule priorities are not supported; however:
 - SNAT rules have higher priority than Masquerade
 - Port Forwarding rules have higher priority than DNAT rules
 - Port Forwarding rules with protocol have higher priority than Port Forwarding rules without protocol

Natting table
-------------

The natting table is used to keep track of the ongoing natting sessions.

Show the natting table:

::

    polycubectl nat1 natting-table show

To flush the natting table:

::

    polycubectl nat1 natting-table del

Flushing the natting table is only useful when you want to add a more specific rule for an already active natting session. After the natting table is flushed, the rule with highest priority is applied.

Examples
--------

Some running examples for various configurations can be found in ``./test/examples``.
