Firewall
========

Implementation details
----------------------

Data plane - fast path
^^^^^^^^^^^^^^^^^^^^^^

Currently eBPF does not support maps with ternary values (i.e., ``wildcard maps``), this forced to implement an algorithm that could offer this functionality and support a large number of rules, the **Linear Bit Vector Search**, that is particularly suitable to be implemented in eBPF and modularized using tail calls, but has an O(NRules) complexity.
A first module parses the packet and sends it to the ingress or egress chain. Each chain has a series of eBPF programs that evaluate one single field, compute the bit vector (in linear time) and sends the packet to the next module. The second-to-last module uses the ``De Bruijn sequence`` to perform a first bit set search, and based on the results calls the next module that performs the actual action on the packet.
Each module is injected only if the rule set requires it (for example, if no rule requires matching on IP source, the module in charge of doing it is not injected).
The rule limit and the O(N) complexity is given by the bit vector computation, that requires a linear search of the array, performed using loop unrolling.

Control Plane
-------------

Code structure
^^^^^^^^^^^^^^

The control plane is in charge of managing each eBPF module. The code has been organized hierarchically to simplify the implementation. The Firewall class acts as a master, it keeps track of all the injected modules. API calls are managed by the ChainRule and Chain classes. Each module is represented in the control plane by a class inheriting from the Program interface, and encapsulates the eBPF module management, offering uniform interfaces to inject or remove the module or interact with its tables. This structure has the advantage of masking a number of MACROS present in the bpf code that are substituted at run-time based on the configuration, for example the number of rules.

Rules computation
^^^^^^^^^^^^^^^^^

The Linear Bit Vector Search requires computing tables of bit vectors, where (in synthesis) each table represent a field, each row represents a value for that field and the matched rules in the form of a bit vector (where the Nth bit is 1 if the rule is matched, 0 if not).
Considering the complexity of the operation, the choice was to compute the tables from zero each time a rule is modified.


