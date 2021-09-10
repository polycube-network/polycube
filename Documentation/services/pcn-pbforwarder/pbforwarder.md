# Policy-Based Forwarder


This service drops, forwards, or sends to the slowpath each packet that matches one of the defined rules, based on the source and destination MAC addresses, VLAN ID, IPv4 addresses, level 4 protocol and ports, and on the input port.
Policy rules can include one or more of the above fields; if a given field is missing, its content is non-influent for the matching.

## Features


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

## Limitations


- Rules numbering
  - Rules number have to be in sequence, starting from 0. For example, a valid rule set has rules with ID 0, 1, 2, ... while a set with rules 0, 1, 3 will not match the rule number 3.

- Unsupported features:
  - More complex actions (e.g., modify field, push/pop VLAN tags)
  - Any field beyond the above list

- The code does not recognize the situation in which a complex rule is deleted in such a way that the remaining rules can leverage a more aggressive optimization (e.g, checking only on L2/L3 fields). Hence, in this case the optimization is not applied.

## How to use


The [example](https://github.com/polycube-network/polycube/tree/master/src/services/pcn-pbforwarder/example) show how to use this service to create a multi-hop forwarding service among different namespaces.
This could be used to see the basic commands of the ``pcn-pbforwarder`` service.


## Implementation details


## Data plane - fast path


The code of the data path is complicated by the fact that currently eBPF does not support maps with ternary values (i.e., wildcard maps). Therefore the algorithm is forced to implement a linear search, checking if the incoming packet matches the first rule, if not it moves to the second rule, and so on.

The matching is done by using a flag, associated to each rule, whose bits are ``1`` in correspondence of the fields that are valid in the rule, and ``0`` if the rule does not consider that field and hence the content of the field can be simply ignored. This introduces a limitation in the current number of rules that can be matched, as the eBPF has to unroll a loop in  order to check all the rules, hence quickly reaching the maximum number of instructions.

In order to (partially) overcome the above limitation, the current data plane code depends on two macros: ``LEVEL`` and ``RULES``, both defined by ``generate_code()`` in ``Pbforwarder.cpp``.
Based on the ``LEVEL`` macro, the fast path enables only the portions of the code that are strictly needed for the matching, based on the fields that actually need to be checked. In order words, if the rules refer only to L2 fields, the fast path enables only the portion of code that handles L2 processing, avoiding unnecessary checks on L3 and L4 fields.
In fact, the ``RULES`` macro keeps the actual number of configured rules, hence it represent the number of times the loop is unrolled by the eBPF compiler.

The flowchart below summarizes the fast path algorithm of the service, which is split in multiple dataplane files ([source folder](https://github.com/polycube-network/polycube/tree/master/src/services/pcn-pbforwarder/src/)).

![Policy-Based Forwarder](pbforwarder.png)

This service does not have any noticeable slow path algorithm for the data plane, which is then omitted here.


### Control path


When a rule is inserted, the corresponding API method is called in ``DpforwarderApi.cpp``. This prepares a struct and the bitmap based on the input received from the user and passes the control to the set_rule method in ``Dpforwarder.cpp``.
The set_rule method evaluates the level required by the rule and sets the attribute ``match_level``, evaluates if the rule is new and sets the attribute `nr_rules`, then invokes `generate_code()` to prepare the eBPF code and loads it.

When the number of rules exceeds the capacity of the data plane, the eBPF compiler returns an error and the user is warned that his latest rule cannot be applied.

![Insert rule](insert-rule.png)


