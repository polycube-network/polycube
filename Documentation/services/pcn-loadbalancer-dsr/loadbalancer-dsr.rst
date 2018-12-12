Load Balancer (DSR)
===================

This service implements a ``Direct Server Return-based`` Load Balancer working at data-link layer.
In this case, Ethernet/IP packets are delivered to the load balancer using its MAC address as destination, which is then replaced with the MAC address of the target server.

Features
--------
- IPv4 support
- Support load balancing of TCP, UDP and ICMP traffic
- Support for a single Virtual IP (VIP)
- Support for ARP reply directed to the VIP address
- Support for two load balancing algorithm(s):  `hash on src_ip` or `hash on session parameters` (sip, dip, sport, dport, proto)
- Support for session persistency across multiple servers, through a session table that keeps track of existing connections
- Replaces original destination MAC address with the MAC address of the actual server

Limitations
-----------
- Cannot support multiple VIPs
- Does not implement any keep alive mechanism to recognize a server failure
- Servers cannot be removed at runtime

How to use
----------

Configure backend servers
^^^^^^^^^^^^^^^^^^^^^^^^^
You need to set the VIP on a loopback interface of the server and disable ARP responder (only on loopback interface):

::

    sudo ifconfig lo 10.0.0.100 netmask 255.255.255.255 up
    sudo sysctl -w net.ipv4.conf.all.arp_ignore=1
    sudo sysctl -w net.ipv4.conf.all.arp_announce=2


Use the load balancer
^^^^^^^^^^^^^^^^^^^^^

::

    # create loadbalancer
    polycubectl lbdsr add lb1

    # add and connect ports
    polycubectl lbdsr lb1 ports add frontend-port type=FRONTEND
    polycubectl lbdsr lb1 ports add backend-port type=BACKEND
    polycubectl lbdsr lb1 ports frontend-port set peer="veth1"
    polycubectl lbdsr lb1 ports backend-port set peer="veth2"

    # setup frontend vip and mac addresses
    polycubectl lbdsr lb1 frontend set mac=aa:bb:cc:dd:ee:ff
    polycubectl lbdsr lb1 frontend set vip=10.0.0.100

    # configure backend server pool
    polycubectl lbdsr lb1 backend pool add 1 mac=01:01:01:01:01:01
    polycubectl lbdsr lb1 backend pool add 2 mac=02:02:02:02:02:02
    polycubectl lbdsr lb1 backend pool add 3 mac=03:03:03:03:03:03

