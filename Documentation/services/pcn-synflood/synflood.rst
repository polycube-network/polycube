SYN Flood Monitor
=================

This service exports some metrics that can be used to detect a possible SYN Flood attack.


Features
--------
- Retrieves a set of TCP/IP parameters that can be used to detect SYN flooding attacks


Limitations
-----------
- It must be launched on the host that needs to be monitored. It cannot operate as a 'main in the middle' mode, i.e., inspecting network traffic directed toward a remote host.


How to use
----------
Technically, ``pcn-synflood`` is a transparent service, hence it should be attached to an existing network interface (e.g., netdev or a virtual link between Polycube services).
However, given that the current implementation retrieves traffic statistics using the metrics provided by the operating system, it can be even instantiated without attaching it to any network interface.


Exported metrics
----------------
- ``tcpAttemptFails``: number of failed TCP connections [1].
- ``tcpOutRsts``: number of TCP segments sent, containing RST flag.
- ``deliverRatio``: ratio between the number of IP pkts delivered to application protocols and the total number of received pkts.
- ``responseRatio``: ratio between the number of IP pkts requests to send by application protocols and the total number of received pkts.


Additional details
------------------

[1] ``tcpAttemptFails``

It measures the number of times TCP connections have made a direct transition to the CLOSED state from either the SYN-SENT state or the SYN-RCVD state, plus the number of times TCP connections have made a direct transition to the LISTEN state from the SYN-RCVD state. It refers to variable ``tcpAttemptFails`` documented in RFC 1213.

It is worth mentioning that this parameter is rather general and can be used to check for unusual situations on both client and server side. For instance, it can be used to detect either (1) that a server is under attack (e.g., TCP state machine goes from SYN-RCVD to LISTEN or CLOSED), or (2) that a client is currently attacking a remote target (e.g., TCP state machine goes from SYN-SENT to CLOSED).

The most common case, i.e., that a server is under attack, corresponds at least to the following unusual TCP sequences:
 - [SYN, timeout]. The server receives a SYN packet, but it cannot answer any more because it is overwhelmed. This connection will be ended after server time‐out, as described earlier.
 - [SYN (Client, Server), RST (Server, Client)]. This sequence means either that the server is the victim of a DoS attack because it cannot reply to the legitimate client any more, or that there is not applications listening on that port.
 - [SYN, SYN/ACK, timeout]. The server waits indefinitely for the ACK packet, either because the IP source address is spoofed or because the ACK packet is rejected because of network congestion. This sequence can correspond to a DoS attack. This connection will be ended after server time‐out.
 - [SYN, SYN/ACK, RST]. This handshake sequence can correspond to a DDoS attack. At the reception of the SYN/ACK packet, the client host then transmits an RST packet to the server because it never sent a SYN packet.
