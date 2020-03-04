Packetcapture service
=====================

Packetcapture is a transparent service that allows to capture packets flowing through the interface it is attached to, apply filters and obtain capture in *.pcap* format. In particular, the service supports either saving captured packets in the local filesystem (e.g., useful in case of high network traffic) or it can interact and deliver packets to a remote client that stores them in the remote filesystem.

An example of a client that uses the REST api of the packetcapture service is available in '*Packetcapture_Client*' directory.

Features
--------

- Transparent service, can be attached to any interface of any Polycube service
- Support for filters (i.e., source prefix, destination prefix, source port, destination port, layer 4 protocol, etc.).
- Support partial capture of packets (i.e., snaplen)
- Support localmode (store data locally) or network mode (send packets to a remote client) operations

Limitations
-----------

- Traffic is returned as is, without any anonimization primitive.


How to use
----------
The packetcapture service is a transparent service, it can be attached to a cube port.

Create the service
^^^^^^^^^^^^^^^^^^

::

    #create the packetcapture service
    polycubectl packetcapture add mysniffer capture=bidirectional

This service can operate in four working modes (actually, the forth mode is just to turn the capture off):

- capture only incoming packets: **capture=ingress**
- capture only outgoing packets: **capture=egress**
- capture both incoming and outgoing packets: **capture=bidirectional**
- turn packet capture off: **capture=off**

*capture* option indicates the direction of the packets that the service must capture.
The direction of the captured packets is independent of the operation in "network mode" or "non network mode".

In this example the service named '*mysniffer*' will work in bidirectional mode.


Attach to a cube port
^^^^^^^^^^^^^^^^^^^^^

::

    # Attach the service to a cube port
    polycubectl attach mysniffer br1:toveth1

Now the packetcapture service is attached to the port *toveth1* of the bridge *br1*


 veth1 ---**x**- |   br1    | ------ veth2    



Filter
-------
Traffic can be selected by adding filters with syntax (tcpdump like):

    polycubectl <service name> set filter=<string value>

-if the filter contains only one word you can put it normally
-if the filter contains more than a word you have to put the string inside the ""

-if you want to capture all the traffic you can put as filter: all

    polycubectl <service name> set filter=all

- default filter captures no packets (the eBPF datapath simply returns ok)

Filter can be viewed using the command **polycubectl mysniffer filter show**
Snaplen can be viewed using the command **polycubectl mysniffer snaplen show**

For further details of the implementation of the filter see :doc:`Packetcapture filter <packetcapture-filter>`

For more details about the filters supported by libpcap (hence, the syntax allowed to specify filters) see `pcap-filter <https://linux.die.net/man/7/pcap-filter>`__


Examples of possible filters
----------------------------

::

    # Example of the source prefix filter
    polycubectl mysniffer set filter="ip src 10.0.2.11"

::

    # Example of the source port filter
    polycubectl mysniffer set filter="src port 80"


::
    
    # Example of the layer 4 protocol filter
    polycubectl mysniffer set filter=tcp

::
    
    # Example of the snaplen filter
    # In this case we capture only the first 80 bytes of each packet
    polycubectl mysniffer set snaplen=80


Get the capture dump
--------------------
When the service is not set in *networkmode*, the dump is automatically written in a resilient way in the temporary user folder.

The path of the capture file can be shown using the command: **polycubectl mysniffer show dump**

Otherwise, if the service is set in network mode, the capture file can be requested through the use of the provided Python client, or queried simply through the service API.


How to use the demo client
^^^^^^^^^^^^^^^^^^^^^^^^^^
::
    
    # Start the client script
    python3 client.py <IPv4 address> <file destination name>


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
    L4:	 return pcn_pkt_controller(ctx, md, reason);
    L5:	 return RX_OK;
