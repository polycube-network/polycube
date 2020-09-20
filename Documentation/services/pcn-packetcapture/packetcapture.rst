Packet capture service 
======================

The Packet Capture (``packetcapture``) is a transparent service that allows to capture packets flowing through the interface it is attached to, apply filters and obtain capture in ``pcap`` format. In particular, the service supports either saving captured packets in the local filesystem (e.g., useful in case of massive load) or it can deliver packets to a remote client that stores them in the remote filesystem.

An example of a client that uses the REST api of the ``packetcapture`` service is available in the `packet capture client  <https://github.com/polycube-network/polycube/tree/master/src/services/pcn-packetcapture/client>`_ folder.

Features
--------

- Transparent service, can be attached to any physical/virtual interface or any Polycube service
- Support for filters (i.e., source prefix, destination prefix, source port, destination port, layer 4 protocol, etc.).
- Support partial capture of packets (i.e., snaplen)
- Support *local mode* (store data locally) or *network mode* (send packets to a remote client) operations.

Limitations
-----------

- Traffic is returned as is, without any anonimization primitive.


How to use
----------
The packetcapture service is a transparent service, it can be attached either to a netdev or to a cube port.

Create the service
^^^^^^^^^^^^^^^^^^

::

    #create the packetcapture service
    polycubectl packetcapture add mysniffer capture=bidirectional

The ``capture`` attribute indicates the direction of the packets that the service must capture. Four values are allowed:

- capture only incoming packets: **capture=ingress**
- capture only outgoing packets: **capture=egress**
- capture both incoming and outgoing packets: **capture=bidirectional**
- turn packet capture off: **capture=off**

The direction of the captured packets is independent of the operation in *network mode* or *local mode*.

In this example the service named ``mysniffer`` will work in bidirectional mode.


Attach to a cube port
^^^^^^^^^^^^^^^^^^^^^

::

    # Attach the service to a cube port
    polycubectl attach mysniffer br1:toveth1

Now the packetcapture service is attached to the port ``toveth1`` of the bridge ``br1``:

::

                             +----------+
   veth1 ---**mysniffer**--- |   br1    | ------ veth2    
                             +----------+



Set a filter
------------
Traffic can be selected by adding filters with a libpcap-compatible syntax:

::

    polycubectl <service name> set filter=<string value>

Filter can be set in this way:

- if the filter contains only **one word**: write it normally

- if the filter contains **more than one word**: write the string inside *double quotes* (i.e., " ")

- if you want to capture **all traffic**: write the *all* keyword as string specifier:

::

    polycubectl <service name> set filter=all

- **default filter**: the service captures no packets (the eBPF datapath simply returns ok)

The currently active filter can be viewed using the command **polycubectl mysniffer filter show**.
The current ``snaplen`` can be viewed using the command **polycubectl mysniffer snaplen show**.

For further details of the implementation of the filter look at the `Implementation details`_ section.

For more details about the filters supported by libpcap (hence, the syntax allowed to specify filters) see the `pcap-filter Linux man page <https://linux.die.net/man/7/pcap-filter>`_.


Examples of possible filters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    # Example of the source prefix filter
    polycubectl mysniffer set filter="ip src 10.0.2.11"

    # Example of the source port filter
    polycubectl mysniffer set filter="src port 80"

    # Example of the layer 4 protocol filter
    polycubectl mysniffer set filter=tcp

    # Example of the snaplen filter
    # In this case we capture only the first 80 bytes of each packet
    polycubectl mysniffer set snaplen=80


Get the capture dump
--------------------
When the service is not set in *networkmode*, the dump is by default written in a resilient way in the temporary user folder.
The folder where the dump is written can be changed by using the syntax:

::

    polycubectl <service name> set dump="<string value>"

    # Example of new dump folder
    polycubectl mysniffer set dump="/home/user_name/Desktop/capture"

The file extension ``.pcap`` will be added at the end of the file name.

If a file with the same name already exists, it will be overwritten. 

The path of the capture file can be shown using the command ``polycubectl mysniffer show dump``.

Otherwise, if the service is set in *network mode*, the capture file can be requested through the use of the provided Python client, or queried simply through the service API.


How to use the packetcapture client
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
::
    
    # Start the client script
    python3 client.py <IPv4 address> <local_dump_name>


Set network mode
^^^^^^^^^^^^^^^^
::
    
    # Start sniffer in network mode
    polycubectl mysniffer set networkmode=true

    # Start sniffer in local model
    polycubectl mysniffer set networkmode=false


Implementation details
----------------------
The pipeline to convert into C code the filtering string entered in the packetcapture service is the following:

**pcap filter** → *libpcap* → **cBPF** → *cbpf2c* → **C code**

More in details, the first step is to obtain the cBPF (assembly) code from the filtering string, using the ``libpcap``/``tcpdump`` format. The filtering string is read from ``polycubed`` REST interface, then it is compiled in cBPF using the ``pcap_compile_nopcap()`` function that returns a ``bpf_program`` structure containing a list of ``bpf_insn``.

Then, the code creates a ``sock_fprog`` structure called ``cbpf`` that contains all the required filter blocks.

The second step (traslation from cBPF to C) starts with the validation of the cBPF code.
Function ``_cbpf_dump()`` is called for each filtering block and it returns a string containing the equivalent C code for that block.

Inside ``_cbpf_dump()``, a switch statement creates two variables, ``op`` (operation) and ``fmt`` (operand) depending on the type of instruction of the block (e.g.,return, load, store, alu op. etc.); the above variables will be used to generate the final C code.

This ASM-to-C traslator is ispired to a similar project proposed by `Cloudflare <https://blog.cloudflare.com/xdpcap/>`_; however, in Polycube the translator is written in C/C++ (the CLoudfare one is in Go); furthermore, in Polycube the final output of the translator is a C equivalent of the packet filter, while in the latest version of the Cloudfare project, the final outcome of the translation are eBPF assembly instructions.

The C output facilitates any further modification of the code, e.g., with when additional processing steps are needed, although it impacts on the overall filter conversion time as it requires one additional processing pass involving CLANG/LLVM to convert the C code into eBPF assembly.


Example of C code generated
^^^^^^^^^^^^^^^^^^^^^^^^^^^
As a example, we list here is the generated C code for the filter ``icmp``:

::

    L0:	 if ((data + 14) > data_end) {
           return RX_DROP;
         }
         a = ntohs(* ((uint16_t *) &data[12]));
    L1:	 if (a == 0x0800) {
           goto L2;
         } else {
           goto L5;
         }
    L2:	 if ((data + 24) > data_end) {
           return RX_DROP;
         }
         a = * ((uint8_t *) &data[23]);
    L3:	 if (a == 0x01) {
           goto L4;
         } else {
           goto L5;
         }
    L4:  pcn_pkt_controller(ctx, md, reason);
    L5:	 return RX_OK;
