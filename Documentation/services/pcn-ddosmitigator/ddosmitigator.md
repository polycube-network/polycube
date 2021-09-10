# DDoS Mitigator


This service implements a DDoS Mitigator, which can drop (malicious) packets at very high speed based on a blacklist applied on either IP source or destination addresses.
Instead, non-IP traffic (e.g., IPv6, ARP, etc.) is always forwarded.

## Features


Supported features:
 - blacklist of source IP addresses (``blacklist-src``)
 - blacklist of destination IP addresses (``blacklist-dst``)
 - ``statistics`` about dropped traffic

## Limitations


- IPv6: currently unsupported; all the IPv6 traffic is forwarded as is, without any check.

## Performance


Although this service can attach to either the ``TC``, ``XDP_SKB`` and ``XDP_DRV`` eBPF hooks, we suggest to use ``XDP_DRV`` (if supported by your NIC driver) in order to get the highest dropping rate.
This loads the service as an ``XDP`` program in driver mode, hence discarding packets as soon as they arrive in the NIC driver, before delivering them to the main networking components of the operating system.

To further improve the performance of this service, the code implementing the datapath is dynamically created (and re-injected in the kernel) in a way that includes only the required features.
For example, the code that handles IP destination address is not present if the user does not ask for IP destination filtering.
This enables this service to create a datapath program that includes only the minimum lines of code required to implement the requested features, with a positive impact on the performance of the system.
