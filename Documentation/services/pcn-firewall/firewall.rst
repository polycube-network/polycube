Firewall
========

This service implements a transparent firewall. It can be attached to a port or a netdev, and it may drop or forward each packet that matches one of the defined rules, based on the source and destination IPv4 addresses, level 4 protocol and ports, and TCP flags.
Policy rules can include one or more of the above fields; if a given field is missing, its content does not influence the matching.

**Non-IP packets are always forwarded, without any check**.


Features
--------

Supported features:

  - Matching on following fields:

    - IPv4 source/destination (with prefix match)
    - L4 protocol (TCP/UDP/ICMP)
    - L4 source/destination port
    - TCP Flags
    - Connection tracking status

  - Possible actions:

    - Forward packet from the interface from which it was received to the other
    - Drop packet

  - Non-IP packets are always forwarded
  - Up to 5k rules for each chain (INGRESS/EGRESS)


How to use
----------

Ingress ad egress chains
^^^^^^^^^^^^^^^^^^^^^^^^

The service supports independent ingress and egress policy chains, with two different policy sets.


Rule insertion
^^^^^^^^^^^^^^

Rule insertion is guaranteed to be *atomic*: during the computation of the new datapath, the old rule set is used until the new rule set is ready, and only at that moment the new policies will be applied.

Rule insertion is an expensive operation. For this reason, we have thought about different endpoints to optimize expensive operations:

  - ``/insert``, ``/delete``, ``/append`` and ``PUT`` on ``rule/<id>`` (update): these endpoints are used to perform a single operation on a rule. As soon as the rule-set is updated, it is compiled and all the modifications are immediately inserted in the datapath.
  - ``/batch``: as suggested by the name, this endpoint is used to perform multiple operation on a single HTTP request. Instead of compiling the new rule-set as soon as a single operation is fulfilled, it waits for all the actions described in the request to be executed. Finally, a single compilation is performed and the datapath is updated once.

Concerning the batch endpoint, it accepts a JSON list of rules like:

.. code-block:: bash

    {
      "rules": [
        {"operation": "insert", "id": 0, "l4proto":"TCP", "src":"192.168.1.1/32", "dst":"192.168.1.10/24", "action":"drop"},
        {"operation": "append", "l4proto": "ICMP", "src":"192.168.1.100/32", "dst":"192.168.1.100/24", "action":"drop"},
        {"operation": "update", "id": 0, "l4proto":"TCP", "src":"192.168.1.2/32", "dst":"192.168.1.20/24", "action":"forward"},
        {"operation": "delete", "id": 0},
        {"operation": "delete", "l4proto":"ICMP", "src":"192.168.1.100/32", "dst":"192.168.1.100/24", "action":"drop"}
      ]
    }


As you can see, every element of the ``rules`` array MUST contain an operation (insert, append, update, delete) plus a rule/id which is the actual target.
All the listed operation are performed sequentially, meaning that the user must sent the operation already ordered as he wants. Pay attention when sending some DELETE with other INSERT, you have to take in mind that during such operations IDs may vary (increase or decrease).

This features is also available from the ``polycubectl`` command line. It is strongly suggested to create a JSON file containing the batch of rules and then type:

``polycubectl firewall <fwname> chain <chainname> batch rules= < filename.json``

Using the redirection diamond you are able to insert the file content in the body of the HTTP POST request generated from the command.

Default action
^^^^^^^^^^^^^^

The default action if no rule is matched is forward. This can be changed for each chain independently by issuing the command
``polycubectl firewall fwname chain INGRESS set default=DROP`` or ``polycubectl firewall fwname chain EGRESS set default=DROP``.

Statistics
^^^^^^^^^^

The service tracks the number of packets and the total bytes that have matched each rule. It is possible to show them by issuing the command ``polycubectl firewall fw chain INGRESS stats show``, follow the help for further details. To flush all the statistics (i.e. both packets and bytes count for every rule) about a chain, issue the following command ``polycubectl firewall fw chain INGRESS reset-counters``.

Examples
--------

The `examples source folder <https://github.com/polycube-network/polycube/tree/master/src/services/pcn-firewall/examples/>`_ contains some simple scripts to show how to configure the service.

Also under the test directory, there are plenty of scripts that test the firewall using both single and batch rule insertion/deletion.


Implementation details
----------------------

Data plane - fast path
^^^^^^^^^^^^^^^^^^^^^^

Currently eBPF does not support maps with ternary values (i.e., *wildcard maps*), this forced to implement an algorithm that could offer this functionality and support a large number of rules, the **Linear Bit Vector Search**, that is particularly suitable to be implemented in eBPF and modularized using tail calls, but has an O(NRules) complexity.

A first module parses the packet and sends it to the ingress or egress chain. Each chain has a series of eBPF programs that evaluate one single field, compute the bit vector (in linear time) and sends the packet to the next module. The second-to-last module uses the *De Bruijn sequence* to perform a first bit set search, and based on the results calls the next module that performs the actual action on the packet.

Each module is injected only if the rule set requires it (for example, if no rule requires matching on IP source, the module in charge of doing it is not injected).
The rule limit and the O(N) complexity is given by the bit vector computation, that requires a linear search of the array, performed using loop unrolling.

An overview of the algorithm is depicted in the figure below.

.. image:: datapath.png
    :align: center


Control Plane
-------------

Code structure
^^^^^^^^^^^^^^

The control plane is in charge of managing each eBPF module. The code has been organized hierarchically to simplify the implementation. The Firewall class acts as a master, it keeps track of all the injected modules. API calls are managed by the ChainRule and Chain classes. Each module is represented in the control plane by a class inheriting from the Program interface, and encapsulates the eBPF module management, offering uniform interfaces to inject or remove the module or interact with its tables. This structure has the advantage of masking a number of MACROS present in the bpf code that are substituted at run-time based on the configuration, for example the number of rules.

Rules computation
^^^^^^^^^^^^^^^^^

The Linear Bit Vector Search requires computing tables of bit vectors, where each table represent a field, each row represents a value for that field and the matched rules in the form of a bit vector (where the Nth bit is 1 if the rule is matched, 0 if not).
Considering the complexity of the operation, the choice was to compute the tables from zero each time a rule is modified.


