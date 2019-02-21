Cubes Architecture
==================

A ``Cube`` is a logical entity, composed by a ``data plane`` (which can include one or more eBPF programs) and a ``control/management plane``.

In addition, a Cube may include also a ``slow path`` in case some additional data plane-related code is required to overcome some limitations of the eBPF data plane.

Types of Cubes
--------------

Polycube has two diferent kind of cubes, **standard cubes** and **transparent cubes**.

Standard cubes have forwarding capabilities, such router and bridge.

A standard cube:
  - defines a :ref:`ports <cube-ports>` concept, ports are *connected* to other ports or netdevs;
  - makes a forwarding decision, i.e, sending the packet to another port;

::

             standard cube
           +--------------+
           |              |
           | +----------+ |
   port1---|-|          |-|---port3
           | |   core   | |
   port2---|-|          |-|---port3
           | +----------+ |
           |              |
           +--------------+

A trasparent cube is a cube without any forwarding capability, such as a network monitor, NAT and a firewall.

A trasparent cube:
  - does not define any port, hence cannot be *connected* to other services, but has to be *attached* to an existing port such as the one of a normal service (e.g., port1 on router2) or a network interface in the host (e.g., veth0);
  - it *inherits* all the parameters associated to ports (e.g., MAC/IPv4 addresses, link speed, etc.) from the actual port it is attached to (e.g., port 1 on router2, or veth0);

::

        transparent cube
      +-------------------+
      |    +---------+    |
      |  ->| ingress |->  |
      | /  +---------+  \ |
 <--->|*                 *|<--->
      | \  +---------+  / |
      |  <-| egress  |<-  |
      |    +-------- +    |
      +-------------------+

Following is example topology composed by standard and transparent cubes.

::

     veth1                                                                 veth3
       |                                                                    |
       |                                                                    |
       |                                                                    |
  +---------+                      +---------+           +---------+   +---------+
  |         |   +-----------+------+         |   +-------+         |   |         |
  | bridge1 |---| firewall0 | nat0 | router1 |---| ddos0 | router2 |---| bridge2 |---veth5
  |         |   +-----------+------+         |   +-------+         |   |         |
  +---------+                      +---------+           +---------+   +---------+
      |                                                                     |
      |                                                                     |
      |                                                                     |
    veth2                                                                 veth4

``polycubectl ?`` shows available cubes installed on your system.

Cubes structure
---------------

Cubes Instances
^^^^^^^^^^^^^^^

Cubes are created by the ``polycubectl <cubetype> add <cubename>`` command, for example:
::

  # create a router instance called r1
  polycubectl router add r1
  # create a simplebridge instance br1
  polycubectl simplebridge add br1


.. _cube-ports:

Cubes Ports
^^^^^^^^^^^
Cubes send/receive traffic through ``ports``.

However, ports are just logical elements and need to be connected to (physical/virtual) network interfaces or to the port of another cube to be fully operational.

In order to send/receive traffic, the user has to setup the ``peer`` for such port. The action of setup the port peer is equivalent to connect it to a network device. More details next.

::


                 +----------+                                     +---------+
     port1 ------|    r1    |------- port2----------port2---------|   br1   |
    (netdev)     |  (cube)  |     (cube port)     (cube port)     |  (cube) |
    peer=veth1   +----------+   peer=br1:port2   peer=r1:port1    +---------+
                       |
                       |
                     port3
                    (netdev)
                   peer=eth0

Ports are created using the ``polycubectl <cubename> ports add <portname> [parameter=value, [parameter1=value1, ...]]`` command.

For instance:
::

  # create port2 in br1 (simplebridge), it doesn't require any parameters
  polycubectl br1 ports add port2

  # router's ports require ip and netmask parameters
  polycubectl r1 ports add port1 ip=10.0.1.1 netmask=255.255.255.0
  polycubectl r1 ports add port2 ip=10.0.2.1 netmask=255.255.255.0
  polycubectl r1 ports add port1 ip=10.0.3.1 netmask=255.255.255.0

Ports Peer
^^^^^^^^^^

The ``peer`` parameter defines where the port is connected to, it is possible to connect ports to linux netdevs or ports belonging to other cubes.

- set peer to a ``netdev`` name in order to connect to it, (``eth0``, ``wlan0``, ``veth1``...)
- set peer to ``cube_name:port_name`` to connect the port to the port of another cube. (e.g. ``br1:port1``). In this case the peer on both ports have to be set in order to create the connection.

If the ``peer`` is empty it means the port is down, so packets are not received from it, and packets sent through it are dropped.

Following is an example of how to set ports peer, referred to previous picture.
::

  polycubectl r1 ports port1 set peer=veth1
  polycubectl r1 ports port3 set peer=eth0

  #In this case both peers must be setup
  polycubectl r1  ports port2 set peer=br1:port2
  polycubectl br1 ports port2 set peer=r1:port2

Connect and  Disconnect primitives
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Connect and disconnect API provides a short way to connect ports, without setting up peers explicitly.

Attach and Detach primitives
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

These primitives allow to associate transparent cubes to standard cube's ports or to netdevs on the system.

::

  polycubectl attach firewall1 r1:port2

  polycubectl attach firewall0 veth1
