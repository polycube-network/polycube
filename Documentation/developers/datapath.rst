Writing the eBPF datapath
^^^^^^^^^^^^^^^^^^^^^^^^^

`eBPF <http://cilium.readthedocs.io/en/latest/bpf/>`_ is an extension of the traditional Berkeley Packet Filter.
The Polycube architecture leverages the software abstraction provided by `BCC <https://github.com/iovisor/bcc/>`_, which is further extended in this project particular with respect to eBPF features that are useful for networking services.
In order to get more information about how to use the maps in BCC please read the `BCC reference guide <https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md>`_, additionally there is a list of the `available eBPF helpers <https://github.com/iovisor/bcc/blob/master/docs/kernel-versions.md>`_.

Polycube architecture adds a wrapper around the user's code, this wrapper calls the `handle_rx` function with the following parameters:

1. **ctx**: Packet to be processed
2. **md**: packet's metadata:

 - **in_port**: integer that identifies the ingress port of the packet.

``polycube`` provides a set of functions to handle the packets, the return value of the `handle_rx` function should be the result of calling one of these functions.

- **pcn_pkt_redirect(struct __sk_buff *skb, struct pkt_metadata *md, u16 port);**: sends the packet through an the ``ifc`` port. [Example](services/pcn-helloworld/src/Helloworld_dp.h#L86)

- **pcn_pkt_drop(struct __sk_buff *skb, struct pkt_metadata *md);**: drops the packet. It is the same that just returning `RX_DROP`. [Example](services/pcn-helloworld/src/Helloworld_dp.h#L78)

- **pcn_pkt_controller(struct __sk_buff *skb, struct pkt_metadata *md, u16 reason)**: sends the packet to the control path controller. Reason can be used to indicate why the packet is being sent to the custom code running in the control path. If there is not any reason `RX_CONTROLLER` could be directly returned. [Example](services/pcn-helloworld/src/Helloworld_dp.h#L82)

- **pcn_pkt_controller_with_metadata(struct __sk_buff *skb, struct pkt_metadata *md, u16 reason, u32 metadata[3])**: Sends the packet to the custom code running in the control path. In addition to the reason the user can also send some additional medatada.

Checksum calculation
********************

The L3 (IP) and L4 (TCP, UDP) checksums has to be updated when fields in the packets are changed.
``polycube`` provides a set of wrappers of the eBPF helpers to do it:

- **pcn_csum_diff()**: wrapper of `BPF_FUNC_csum_diff <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=7d672345ed295b1356a5d9f7111da1d1d7d65867>`_

    Note: For XDP cubes and kernels version prior to 4.16 this function supports only 4 bytes arguments.

- **pcn_l3_csum_replace()**: wrapper of `BPF_FUNC_l3_csum_replace <https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/commit/?id=91bc4822c3d61b9bb7ef66d3b77948a4f9177954>`_

- **pcn_l4_csum_replace()**: wrapper of `BPF_FUNC_l4_csum_replace <https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/commit/?id=91bc4822c3d61b9bb7ef66d3b77948a4f9177954>`_

Services as :scm_web:`nat <src/services/pcn-nat/src/Nat_dp.c>` and :scm_web:`nat <src/services/pcn-loadbalancer-rp/src/Lbrp_dp.c>` show how to use these functions.

Vlan Support
************

The vlan handling in TC and XDP eBPF programs is a little bit different, so polycube includes a set of helpers to uniform this accross.

- bool pcn_is_vlan_present(struct CTXTYPE *pkt)

- int pcn_get_vlan_id(struct CTXTYPE *pkt, uint16_t *vlan_id, uint16_t *eth_proto);

- uint8_t pcn_vlan_pop_tag(struct CTXTYPE *pkt);

- uint8_t pcn_vlan_push_tag(struct CTXTYPE *pkt, u16 eth_proto, u32 vlan_id);


Known limitations:
******************
- It is not possible to send a packet through multiple ports, then multicast, broadcast of any similar functionality has to be implemented in the control path.

TODO:
*****

- Document support for multiple eBPF programs

Debugging and logging in the data plane
***************************************

The polycube framework provides the ``pcn_log()`` primitive that enables to print messages in the standard log file; hence, all the messages (from both data and control plane) are received in the same place.
This avoids the use of the ``bpf_trace_printk()`` primitive, which is not integrated with control plane logging.

Syntax:

::

    pcn_log(ctx, level, msg, args...)

- `ctx`: packet being processed.
- `level`: type of logging level: ``LOG_TRACE``, ``LOG_DEBUG``, ``LOG_INFO``, ``LOG_WARN``, ``LOG_ERR``, ``LOG_CRITICAL``.
A brief explanation about when to use each level can be found in `this answer <https://stackoverflow.com/questions/2031163/when-to-use-the-different-log-levels>`_ on StackOverflow
- `msg`: message to print
- `args`: arguments of the message

Two special format specifiers are available:
- ``%I``: print an IP address
- ``%M``: print a MAC address
- ``%P``: print a TCP/UDP port

Please note the the custom specifiers spec the data to be in network byte order while standard specifiers expects it to be in host by order.

Current limitations:
 - Cannot be used inside a macro
 - Maximum 4 arguments are allowed

Usage example:

::

  pcn_log(ctx, LOG_DEBUG, "Receiving packet from port %d", md->in_port);


The ``pcn_pkt_log(ctx, level)`` primitive sends a packet to the control plane where it is printed in a tcpdump like format.
`ctx` and `level` are the same as in `pcn_log`.
This feature is only designed for developers, so final version of services should not include this.
`polycube-tools <https://github.com/mauriciovasquezbernal/polycube-tools>`_) must be installed and ``polycube`` cmake must be configured with `cmake -DHAVE_POLYCUBE_TOOLS=ON ..`

Usage example:

::

  pcn_pkt_log(ctx, LOG_DEBUG);

