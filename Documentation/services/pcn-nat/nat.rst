NAT
===

This service implements a NAT, network address translation working at layer 3. In particular, the service can remap source and destination IP addresses and source and destination ports in IP datagram packet headers while they are transit across a traffic routing device.
This is a transparent NAT, so it requires to be used with a router.

Features
--------

- Transparent NAT
- IPv4 support
- Support ICMP, TCP and UDP traffic
- Support for Source NAT, Masquerade, Destination NAT and Port Forwarding

Limitations
-----------

- Unsupported IPv6
- Unsupported ARP: the ARP messages will be forwarded without processing them
- Incremental external port choice, starting from 1024 each time 65535 is reached
- The IP address of the EXTERNAL port must be the same as the IP address of the router port connected to the nat

How to use
----------

::

    #create nat
    polycubectl nat add nat1

    #add ports
    polycubectl nat1 ports add to_router type=INTERNAL
    polycubectl nat1 ports add to_internet type=EXTERNAL ip=external_ip

    #connect ports
    polycubectl connect nat1:to_router router1:to_nat
    polycubectl connect nat1:to_internet veth1


Rules
-----

Adding rules
^^^^^^^^^^^^

Source NAT
**********

Available and mandatory fields for SNAT rules are
- ```internal-net```: the internal network to translate (e.g 10.0.0.0/24, 10.0.0.1/32)
- ```external-ip```: the external IP address (e.g. 1.2.3.4)

Sample rule:

::

    polycubectl nat1 rule snat append internal-net=10.0.0.0/24 external-ip=1.2.3.4

Masquerade
**********

Masquerade can be either enabled or disabled.

When enabled, the source IP address of the packet is set to the IP of the EXTERNAL port of the NAT, and a new source port is assigned.

To enable masquerade:

::

    polycubectl nat1 rule masquerade enable

To disable masquerade:

::

    polycubectl nat1 rule masquerade disable

Destination NAT
***************

Available and mandatory fields for DNAT rules are
- ```internal-ip```: the internal IP address (e.g 10.0.0.1)
- ```external-ip```: the external IP address (e.g. 1.2.3.4)

Sample rule:

::

    polycubectl nat1 rule dnat append internal-ip=10.0.0.1 external-ip=1.2.3.4

All the packets directed to ```1.2.3.4``` will be sent to ```10.0.0.1```, so use this rule carefully.

Port Forwarding
***************

Available and mandatory fields for Port Forwarding rules are
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

It is not possible to manually set a priority for each rule, but:
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
