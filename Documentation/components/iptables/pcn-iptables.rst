pcn-iptables: a clone of iptables based on eBPF
===============================================

Polycube includes the ``pcn-iptables`` standalone application, a stateful firewall whose syntax and semantic are compatible with the well-known ``iptables`` Linux tool.

The frontend provides the same CLI of iptables; users can set up security policies using the same syntax by simply executing ``pcn-iptables`` instead of ``iptables``.
The backend is based on `eBPF` programs, more efficient classificaiton algorithms and runtime optimizations; the backend runs as a dedicated service in Polycube.



Supported features
------------------

Currently supported features:

- Support for ``INPUT``, ``OUTPUT``, ``FORWARD`` chains
- Support for ``ip``, ``protocol``, ``ports``, ``tcp flags``, ``interfaces``
- Support for ``connection tracking``
- Support for bpf ``TC`` and ``XDP`` mode

Detailed supported parameters

- ``-s`` source IP
- ``-d`` destination IP
- ``-p`` l4 protocol
- ``--sport`` source port
- ``--dport`` destination port
- ``--tcpflags`` tcp flags
- ``-i`` input interface
- ``-o`` output interface
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
- No support for ``SNAT``, ``DNAT``, ``MASQUESRADE``
- ``-S`` ``-L`` generate an output slightly different from iptables

Install
-------

Prerequisites
^^^^^^^^^^^^^

pcn-iptables comes as a component of polycube framework.
Refer to :doc:`polycube install guide<../../../installation>` for dependencies, kernel requirements and basic checkout and install guide.

Install Steps
^^^^^^^^^^^^^

To compile and install ``pcn-iptables``, you should enable the ``ENABLE_PCN_IPTABLES`` flag in the polycube CMakeFile, which is set to ``OFF`` by default;
this allows to compile the customized version of ``iptables`` used to translate commands, and install in the system pcn-iptables-init pcn-iptables and pcn-iptables-clean utils.

Note:
The ``ENABLE_SERVICE_IPTABLES`` flag, which is set to ``ON`` by default, is used to compile and install the ``libpcn-iptables.so`` service (like other polycube services: bridge, router, ..).
This flag is required to be enabled as well, but it comes by default.

::

        cd polycube

        # Note: ensure git submodules are updated
        # git submodule update --init --recursive

        mkdir -p build
        cd build
        cmake .. -DENABLE_PCN_IPTABLES=ON
        make -j`nproc` && sudo make install

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

- pcn-iptables operates only on interfaces that support XDP native mode
- traffic is not filtered on interfaces that support only eBPF TC programs.

pcn-iptables components
-----------------------

iptables submodule
^^^^^^^^^^^^^^^^^^

A customized fork of iptables is included as submodule under :scm_web:`src/components/iptables/iptables <src/components/iptables>`.
We customized this version of iptables in order not to inject iptables command into netfilter, but convert them, after a validation step, into polycube syntax.

scripts folder
^^^^^^^^^^^^^^

Scripts are used as a glue logic to make pcn-iptables run. Main purpose is initialize, cleanup and run pcn-iptables, pass pcn-iptables parameters through iptables (in charge of converting them), then pass converted commands to pcn-iptables service.
Scripts are installed under ``/usr/local/bin``.