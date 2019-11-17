Policy-Based Forwarder
======================

This service drops, forwards, or sends to the slowpath each packet that matches one of the defined rules, based on the source and destination MAC addresses, VLAN ID, IPv4 addresses, level 4 protocol and ports, and on the input port.
Policy rules can include one or more of the above fields; if a given field is missing, its content is non-influent for the matching.

Feature list
------------

Supported features:
- Wildcard matching on the above fields:
  - Input port
  - MAC source/destination
  - VLAN ID
  - IPv4 source/destination
  - L4 protocol (TCP/UDP)
  - L4 source/destination port

- Possible actions:
  - Forward packet on a given output port
  - Send packet to slow path
  - Drop packet

Limitations
-----------

- Rules numbering
  - Rules number have to be in sequence, starting from 0. For example, a valid rule set has rules with ID 0, 1, 2, ... while a set with rules 0, 1, 3 will not match the rule number 3.

- Unsupported features:
  - More complex actions (e.g., modify field, push/pop VLAN tags)
  - Any field beyond the above list

- The code does not recognize the situation in which a complex rule is deleted in such a way that the remaining rules can leverage a more aggressive optimization (e.g, checking only on L2/L3 fields). Hence, in this case the optimization is not applied.

Usage example
-------------

TODO: how to expose examples?
The [example](./example) show how to use this service to create a multi-hop forwarding service among different namespaces.
This could be used to see the basic commands of the ``pcn-pbforwarder`` service.


