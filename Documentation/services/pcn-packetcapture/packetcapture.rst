Packetcapture service
=====================

Packetcapture is a transparent service that allows to capture packets flowing through the interface it is attached to, apply (simple) filters and obtain capture in *.pcap* format. In particular, the service supports either saving captured packets in the local filesystem (e.g., useful in case of high network traffic) or it can interact and deliver packets to a remote client that stores them in the remote filesystem.

An example of a client that uses the REST api of the packetcapture service is available in '*Packetcapture_Client*' directory.

Features
--------
- Transparent service, can be attached to any interface of any Polycube service
- Support for (simple) IPv4 filters: source prefix, destination prefix, source port, destination port and layer 4 protocol.
- Support partial capture of packets (i.e., snaplen)
- Support localmode (store data locally) or network mode (send packets to a remote client) operations

Limitations
-----------
- No IPv6 filtering
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
The direction of the captured packets is independent of the operation in "nework mode" or "non network mode".

In this example the service named '*mysniffer*' will work in bidirectional mode.


Attach to a cube port
^^^^^^^^^^^^^^^^^^^^^
::

    # Attach the service to a cube port
    polycubectl attach mysniffer br1:toveth1

Now the packetcapture service is attached to the port *toveth1* of the bridge *br1*


 veth1 ---**x**- |   br1    | ------ veth2    



Filters
-------
Traffic can be selected by means of the following filters:

- source prefix
- destination prefix
- source port
- destination port
- later 4 protocol

Source prefix filter
^^^^^^^^^^^^^^^^^^^^
::

    # Example of the source prefix filter
    polycubectl mysniffer filters set src=10.10.10.10/24

Destination prefix filter
^^^^^^^^^^^^^^^^^^^^^^^^^
::
    
    # Example of the destination prefix filter
    polycubectl mysniffer filters set dst=10.10.10.10/24

Source port filter
^^^^^^^^^^^^^^^^^^
::
    
    # Example of the source port filter
    polycubectl mysniffer filters set sport=80

Destination port filter
^^^^^^^^^^^^^^^^^^^^^^^
::
    
    # Example of the destination port filter
    polycubectl mysniffer filters set dport=80

Layer 4 protocol filter
^^^^^^^^^^^^^^^^^^^^^^^
::
    
    # Example of the layer 4 protocol filter
    polycubectl mysniffer filters set l4proto=tcp

Snaplen filter
^^^^^^^^^^^^^^
::
    
    # Example of the snaplen filter
    #  In this case we capture only the first 80 bytes of each packet
    polycubectl mysniffer filters set snaplen=80


Filters can be viewed using the command **polycubectl mysniffer filters show**

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
