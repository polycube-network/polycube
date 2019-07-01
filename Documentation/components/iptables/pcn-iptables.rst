pcn-iptables: An iptables clone based on eBPF
=============================================

**Disclaimer**: this guide is still a draft

Polycube comes with ``iptables`` application (in brief ``pcn-iptables``) that provides an iptables clone, with compatible syntax and semantic.
The backend is based on `eBPF` programs, more efficient algorithms and runtime optimizations.
The frontend provides same iptables CLI, users can setup security policies using same syntax.

Supported features
------------------

Currently supported features:

- Support for ``INPUT``, ``OUTPUT``, ``FORWARD`` chains
- Support for ``ip``, ``protocol``, ``ports``, ``tcp flags``
- Support for ``connection tracking``
- Support for bpf ``TC`` and ``XDP`` mode

Detailed supported parameters

- ``-s`` source IP
- ``-d`` destination IP
- ``-p`` l4 protocol
- ``--sport`` source port
- ``--dport`` destination port
- ``--tcpflags`` tcp flags
- ``-m conntrack --ctstate`` conntrack module

Detailed supported targets

- ``-j ACCEPT`` accept traffic
- ``-j DROP`` drop traffic

Detailed supported commands

- ``-S`` Show rules
- ``-L`` List rules
- ``-A`` Append rule
- ``-I`` Insert rule
- ``-D`` Delete rule
- ``-P <CHAIN> DROP/ACCEPT`` Setup default policy for ``<CHAIN>``
- ``-F <CHAIN>`` Flush policies for ``<CHAIN>``

Limitations
^^^^^^^^^^^

- No support for multiple chains
- No support for ``-i`` ``-o`` interfaces
- No support for ``SNAT``, ``DNAT``, ``MASQUESRADE``
- ``-S`` ``-L`` generate an output slightly different from iptables

Install
-------

For ``pcn-iptables`` support you should enable ``ENABLE_PCN_IPTABLES`` flag in CMakeFile.
::

        cd polycube/build/
        cmake .. -DENABLE_PCN_IPTABLES=ON
        make -j`nproc` && make install


Run
---

1. Initialize pcn-iptables
^^^^^^^^^^^^^^^^^^^^^^^^^^

::

        # Start polycubed, in other terminal (or background)
        sudo polycubed --daemon
        # Initialize pcn-iptables
        pcn-iptables-init


2. Use pcn-iptables
^^^^^^^^^^^^^^^^^^^

``pcn-iptables`` provides same iptables syntax. Please refer to ``iptables`` online docs for more info.
Following are just few examples of available commands.

::

        # E.g.
        pcn-iptables -A INPUT -s 10.0.0.1 -j DROP # Append rule to INPUT chain
        pcn-iptables -D INPUT -s 10.0.0.1 -j DROP # Delete rule from INPUT chain
        pcn-iptables -I INPUT -s 10.0.0.2 -j DROP # Insert rule into INPUT chain

        # Example of a complex rule
        pcn-iptables -A INPUT -s 10.0.0.0/8 -d 10.0.0.2 -p tcp --sport 9090 --dport 80 --tcpflags SYN,ACK ACK -j DROP

        # Example of a conntrack rule
        pcn-iptables -A OUTPUT -m conntrack --ctstate=ESTABLISHED -j ACCEPT

        # Show rules
        pcn-iptables -S # dump rules
        pcn-iptables -L INPUT # dump rules for INPUT chain

        pcn-iptables -P FORWARD DROP # set default policy for FORWARD chain



**NOTE**: do `not` use use ``sudo pcn-iptables ...``

3. Stop pcn-iptables
^^^^^^^^^^^^^^^^^^^^

::

        # Stop and clean pcn-iptables
        pcn-iptables-clean

	# Execute the below command to validate if cleanup is successful.
	pcn-iptables -S
	``Note:  On successful cleanup, you should receive "No cube found named pcn-iptables"``


Advanced Features
-----------------

XDP mode
^^^^^^^^

``pcn-iptables`` can also be run in ``XDP`` mode. This mode comes with performance gain, especially when policy are configured to DROP traffic.

::

        pcn-iptables-init-xdp

Limitations
^^^^^^^^^^^

- It requires your network interfaces to support XDP Native mode
- If any interface is not supporting XDP, on such interface traffic is not filtered
