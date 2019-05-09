Firewall
========

This service is a transparent firewall, it can be attached to a port or a netdev, and it may drop or forward each packet that matches one of the defined rules, based on the source and destination IPv4 addresses, level 4 protocol and ports, and TCP flags.
Policy rules can include one or more of the above fields; if a given field is missing, its content is influent for the matching.
*Packets that are not ip are forwarded without any check*.

Features
--------

Supported features:
- Matching on following fields:
  - ``IPv4 source/destination`` (with prefix match)
  - ``L4 protocol`` (TCP/UDP/ICMP)
  - ``L4 source/destination port``
  - ``TCP Flags``
  - ``Connection tracking`` status

- Possible actions:
  - ``Forward`` packet from the interface from which it was received to the other
  - ``Drop`` packet

- Not IP packets are forwarded by default.
- Up to 5k rules for each chain (INGRESS/EGRESS).

How to use
----------

Ingress ad egress chains
^^^^^^^^^^^^^^^^^^^^^^^^

The service is based on the ingress and egress chains.
Ingress and egress chains are independent and have two different policy sets.

Rule insertion
^^^^^^^^^^^^^^

Rule insertion is guaranteed to be ``atomic``: during the computation of the new datapath, the old rule set is used until the new rule set is ready, and only at that moment the new policies will be applied.

Rule insertion is an expensive operation. For this reason, there are two modes that can be used based on the needs:
- ``Interactive mode:`` this is the default mode. It makes the commands to modify policies synchronous, so that they return only when the modification is reflected in the datapath. This is the slowest mode, as it requires to recompute the datapath for each command, but it has the advantage that a command returns only when the operation is completed.
- ``Transaction mode:`` in this mode commands on the policies are chained and asynchronously applied to the datapath altogether when the user asks it. The performance gain is sensible when commands have to be issued together (e.g. a set of rules), as it requires only one interaction with the datapath at the end. To switch in the transaction mode, it is necessary to issue the command ``polycubectl firewall fwname set interactive=false``. In this way, rules can be inserted normally, but to apply them the command ``polycubectl firewall fwname chain INGRESS apply-rules`` has to be issued. Notice: this command is specific for the chain, and updates the specified chain only.

Default action
^^^^^^^^^^^^^^

The default action if no rule is matched is drop. This can be changed for each chain independently by issuing the command
``polycubectl firewall fwname chain INGRESS set default=FORWARD`` or ``polycubectl firewall fwname chain EGRESS set default=FORWARD``.

Statistics
^^^^^^^^^^

The service tracks the number of packets and the total bytes that have matched each rule. It is possible to show them by issuing the command ``polycubectl firewall fw chain INGRESS stats show``, follow the help for further details. To flush all the statistics (i.e. both packets and bytes count for every rule) about a chain, issue the following command ``polycubectl firewall fw chain INGRESS reset-counters``.

Examples
--------

The ``/examples`` folder contains some simple script to show how to configure the service.
