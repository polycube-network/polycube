# Writing the eBPF datapath


[eBPF](http://cilium.readthedocs.io/en/latest/bpf/>) is an extension of the traditional Berkeley Packet Filter.
The Polycube architecture leverages the software abstraction provided by [BCC](https://github.com/iovisor/bcc/), which is further extended in this project particular with respect to eBPF features that are useful for networking services.
In order to get more information about how to use the maps in BCC please read the [BCC reference guide](https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md), additionally there is a list of the [available eBPF helpers](https://github.com/iovisor/bcc/blob/master/docs/kernel-versions.md).

Polycube architecture adds a wrapper around the user's code, this wrapper calls the ``handle_rx`` function with the following parameters:

- **ctx**: packet to be processed
- **md**: packet metadata
- **in_port**: integer that identifies the ingress port of the packet.

Polycube provides a set of functions to handle the packets, the return value of the ``handle_rx`` function should be the result of calling one of these functions.

- **pcn_pkt_redirect(struct __sk_buff *skb, struct pkt_metadata *md, u16 port);**: sends the packet through an the ``ifc`` port. [Example: Helloworld service](https://github.com/polycube-network/polycube/blob/master/src/services/pcn-helloworld/src/Helloworld_dp_ingress.c#L96).

- **pcn_pkt_drop(struct __sk_buff *skb, struct pkt_metadata *md);**: drops the packet. It is the same that just returning `RX_DROP`. [Example: Helloworld service](https://github.com/polycube-network/polycube/blob/master/src/services/pcn-helloworld/src/Helloworld_dp_ingress.c#L80).

- **pcn_pkt_redirect_ns(struct __sk_buff *skb, struct pkt_metadata *md, u16 port)**: (available only for *shadow* services) sends the packet to the namespace if it comes from the port indicated as parameter.



## Processing packets in the slowpath


A copy of the packet can be sent to the controller to be processed by the slowpath using the following helpers:

- **pcn_pkt_controller(struct __sk_buff *skb, struct pkt_metadata *md, u16 reason)**: sends a copy of the packet to the controller. Reason can be used to indicate why the packet is being sent to the custom code running in the control path.
[[Example: Helloworld service](https://github.com/polycube-network/polycube/blob/master/src/services/pcn-helloworld/src/Helloworld_dp_ingress.c#L79)].

- **pcn_pkt_controller_with_metadata(struct __sk_buff *skb, struct pkt_metadata *md, u16 reason, u32 metadata[3])**: sends a copy of the packet to the custom code running in the control path. In addition to the reason the user can also send some additional metadata.
 
The packet will be processed by the ``packet_in`` method of the controller.

## Checksum calculation


The L3 (IP) and L4 (TCP, UDP) checksums has to be updated when fields in the packets are changed.
``polycube`` provides a set of wrappers of the eBPF helpers to do it:

- **pcn_csum_diff()**: wrapper of [BPF_FUNC_csum_diff](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=7d672345ed295b1356a5d9f7111da1d1d7d65867)
Note that in case of XDP cubes and kernels version prior to 4.16 this function supports only 4 bytes arguments.

- **pcn_l3_csum_replace()**: wrapper of [BPF_FUNC_l3_csum_replace](https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/commit/?id=91bc4822c3d61b9bb7ef66d3b77948a4f9177954)

- **pcn_l4_csum_replace()**: wrapper of [BPF_FUNC_l4_csum_replace](https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/commit/?id=91bc4822c3d61b9bb7ef66d3b77948a4f9177954)

Services as [NAT](https://github.com/polycube-network/polycube/blob/master/src/services/pcn-nat/src/Nat_dp.c) and [Load Balancer](https://github.com/polycube-network/polycube/blob/master/src/services/pcn-loadbalancer-rp/src/Lbrp_dp.c) show how to use these functions.

## VLAN Support


The VLAN handling in TC and XDP eBPF programs is a little bit different, so polycube includes a set of helpers to uniform this accross.

- bool **pcn_is_vlan_present** (struct CTXTYPE* pkt)
- int **pcn_get_vlan_id** (struct CTXTYPE* pkt, uint16_t* vlan_id, uint16_t* eth_proto);
- uint8_t **pcn_vlan_pop_tag** (struct CTXTYPE* pkt);
- uint8_t **pcn_vlan_push_tag** (struct CTXTYPE* pkt, u16 eth_proto, u32 vlan_id);

## Timestamping


So far we know that computing timestamp in eBPF is quite tricky, due to the lack of usable kernel helpers.
The only supported function is **bpf_ktime_get_ns()**, which returns the value of a kernel MONOTONIC clock, meaning that
if the device has gone to sleep/suspend that clock is not updated. Moreover, in the **ctx** structure the field **ctx->tstamp**
has not a standard behaviour: we tested that this value can be:

- empty
- Unix epoch timestamp
- Unknown (maybe kernel timer) timestamp

We have introduced an helper function to compute such timestamp referring to Unix Epoch: **pcn_get_time_epoch()**.
What basically happens is that to all eBPF program is injected a flag **_EPOCH_BASE** containing a precomputed value. This value
represents the **Unix_Epoch - Kernel_timer** at time X. Calling the helper would increase this value by **bpf_ktime_get_ns()**
nanoseconds, thanks to you obtain the Unix Epoch time of when you call such helper.

In the following release, it will be possible to compute the exact timestamp even though the device sleeps/suspends due to
the introduction of a new **bpf_ktime_get_ns()** function, which in addition will consider the inactivity time (CLOCK_BOOTTIME instead of CLOCK_MONOTONIC).

## Known limitations

- Since you cannot send a packet on multiple ports, multicast, broadcast or any similar functionality has to be implemented in the control path.
- The support for multiple eBPF programs is not yet documented.
- Timestamp will not be exact if the device running Polycube sleeps/suspends during its execution.


## Debugging the data plane

See how to debug by [logging in the dataplane](./debugging.md#logging-the-ebpf-data-plane).

