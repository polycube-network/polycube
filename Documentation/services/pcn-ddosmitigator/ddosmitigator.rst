DDoS Mitigator
==============

This service implements a DDoS Mitigator, which can drop (malicious) packets at very high speed based on a blacklist applied on either IP source or destination addresses.
Instead, non-IP traffic (e.g., IPv6, ARP, etc.) is always forwarded.

Features
--------

Supported features:
 - blacklist of source IP addresses (``blacklist-src``)
 - blacklist of destination IP addresses (``blacklist-dst``)
 - possibility to operate in ``STACK/REDIRECT`` mode (more info later)
 - ``statistics`` about dropped traffic

Limitations
-----------

- IPv6: currently unsupported; all the IPv6 traffic is forwarded as is, without any check.

How to use
----------

Working modes: STACK vs REDIRECT
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This service supports two working modes: ``STACK`` and ``REDIRECT``.

In both modes, the first port attached to ddosmitigator module is called ``active-port``.
Packets coming from the external world and received on this port will be filtered and possibly dropped (if match the blacklists).

After this common behavior, the service processes packets differently according to the operating mode.

In ``STACK`` mode, packets that survive the filtering are passed to the Linux TCP/IP stack, hence can be processed by the standard Linux networking modules (e.g., iptables, etc).

In ``REDIRECT`` mode, packets that survive the filtering are forwarded to another port, such as another cube or a physical/virtual interface.
In this case, the user has to specify also a ``redirect-port`` through the CLI, such as ``polycubectl ddosmitigator ddos1 set redirect-port="eth2"``.
Please note that in this case the DDoS mitigator service has to handle a bidirectional stream, i.e., the traffic that comes from ``redirect-port`` is forwarded to ``active-port`` and vice versa; however, blacklists are only applied to the incoming traffic (i.e., that enter from the ``active-port``).


Performance
-----------

Although this service can attach to either the ``TC``, ``XDP_SKB`` and ``XDP_DRV`` eBPF hooks, we suggest to use ``XDP_DRV`` (if supported by your NIC driver) in order to get the highest dropping rate.
This loads the service as an ``XDP`` program in driver mode, hence discarding packets as soon as they arrive in the NIC driver, before delivering them to the main networking components of the operating system.

To further improve the performance of this service, the code implementing the datapath is dynamically created (and re-injected in the kernel) in a way that includes only the required features.
For example, the code that handles IP destination address is not present if the user does not ask for IP destination filtering.
This enables this service to create a datapath program that includes only the minimum lines of code required to implement the requested features, with a positive impact on the performance of the system.

Examples
--------

Examples are available under ``/examples`` folder.