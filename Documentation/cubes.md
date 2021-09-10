# Cubes Architecture


A ``Cube`` is a logical entity, composed by a ``data plane`` (which can include one or more eBPF programs) and a ``control/management plane``.

In addition, a Cube may include also a ``slow path`` in case some additional data plane-related code is required to overcome some limitations of the eBPF data plane.
This is the case of flooding packets in an 802.1 bridge, when the MAC destination address does not exist (yet) in the filtering database.
In fact, this action (which requires a packet to be sent on all active ports except the one on which it was received) involves redirecting a packet on multiple interfaces, an operation that is not supported by the eBPF technology.
In addition, this operation may require a loop in the dataplane, i.e., cycling on the list of interfaces and send them the packet, which is hard to do due to the eBPF limitations.

## Types of Cubes


Polycube has two diferent kind of cubes, **standard cubes** and **transparent cubes**.

Standard cubes have forwarding capabilities, such router and bridge.

A standard cube:
  - defines a [ports](#cubes-structure) concept, ports are *connected* to other ports or netdevs;
  - makes a **forwarding** decision, i.e, sending the packet to another port;
  - it follows middle-box model, is a network function with multiple ports;

```
             standard cube
           +--------------+
           |              |
           | +----------+ |
   port1---|-|          |-|---port3
           | |   core   | |
   port2---|-|          |-|---port4
           | +----------+ |
           |              |
           +--------------+
```

A transparent cube is a cube without any forwarding capability, such as a network monitor, NAT and a firewall.

A transparent cube:
  - does not define any port, hence it cannot be *connected* to other services, but *attached* to an existing port;
  - it can be *attached* to an existing port such as the one of a normal service (e.g., port1 on router2) or a network device (netdev) in the host (e.g., `veth0` or `eth0`);
  - it *inherits* all the parameters associated to the port it is attached to (e.g., MAC/IPv4 addresses, link speed, etc.);
  - it allows to have a *stack* of transparent services on top of a port, very similar to a stack of functions.

A transparent cube can define two processing handlers, *ingress* and *egress*, which operate on two possible traffic directions:
  - *ingress*: it handles the traffic that goes *toward* the port it is attached to. In case of a netdev (e.g., `eth0`), this selects the traffic that comes from the external world and goes toward the TCP/IP stack of the host. In case of a cube port, it is the traffic that is *entering* in the network function.
  - *egress*: it handles the traffic that comes from the cube/netdev it is attached to. In case of a netdev (e.g., `eth0`), it selects the traffic that comes from the TCP/IP stack and *leaves* the host from that port. In case of a cube port, it is the traffic that is *leaving* the network function.

```
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
```

Following is example topology composed by standard and transparent cubes.

```
     veth1                                                                   veth3
       |                                                                       |
       |                                                                       |
       |                                                                       |
  +---------+                       +---------+            +---------+    +---------+
  |         |    +-----------+------+         |    +-------+         |    |         |
  | bridge1 |----| firewall0 | nat0 | router1 |----| ddos0 | router2 |----| bridge2 |---veth5
  |         |    +-----------+------+         |    +-------+         |    |         |
  +---------+                       +---------+            +---------+    +---------+
      |                                                                        |
      |                                                                        |
      |                                                                        |
    veth2                                                                    veth4
```

``polycubectl ?`` shows available cubes installed on your system.


A shadow cube:

  Only a standard cube can be **Shadow** type;
   - ``polycubectl <cubetype> add <cubename> shadow=true``.

  A shadow cube is associated with a Linux network namespace;

  The parameters between the shadow cube and the namespace are aligned;

  A port defined on a shadow cube is also visible from the network namespace:
   - the user can decide to configure the ports using Linux (e.g. ifconfig or the ip command) or polycubectl;

     for example: "``polycubectl <cubename> ports <PortName> set ip=<IpAddress>``" it is the same as "``ip netns exec pcn-<cubename> ifconfig <PortName> <IpAddress>``".
   - the developer can let Linux handle some traffic by sending it to the namespace (e.g. ARP, ICMP, but in general all those protocols able to be managed by a tool running inside the namespace);

```
                       +--------------+
               port1---|              |---port3
                       |  namespace   |
               port2---|              |---port4
  Linux                +--------------+
 ____________________________________________________________
```

```
  Polycube               shadow cube
                       +--------------+
                       |              |
                       | +----------+ |
               port1---|-|          |-|---port3
                       | |   core   | |
               port2---|-|          |-|---port4
                       | +----------+ |
                       |              |
                       +--------------+
```

## Cubes structure


### Cubes Instances


Cubes are created by the ``polycubectl <cubetype> add <cubename>`` command, for example:

```
# create a router instance called r1
polycubectl router add r1
# create a simplebridge instance br1
polycubectl simplebridge add br1
```


### Create Ports

Cubes can send/receive traffic through ``ports``.

NOTE: Just create a port does NOT allow to send/receive traffic.

Ports are created using ``polycubectl <cubename> ports add <portname> [parameter=value, [parameter1=value1, ...]]``.

In order to ``send/receive traffic``, the user has to setup the ``peer`` value or use the equivalent ``connect`` primitive. More details next.

Ports are logical entities and need to be connected to (physical/virtual) network interfaces or to other ports to be fully operational.

```
                 +----------+                                     +---------+
     port1 ------|    r1    |------ port2-----><-------port2------|   br1   |
    (netdev)     |  (cube)  |    (cube port)        (cube port)   |  (cube) |
    peer=veth1   +----------+  peer=br1:port2      peer=r1:port2  +---------+
                       |
                       |
                     port3
                    (netdev)
                   peer=eth0
```

For instance:

```
# create port2 on br1 (simplebridge), it doesn't require any further parameters
polycubectl br1 ports add port2

# create portX on r1 (router), it doesn't require mandatory parameters, but it is useful to assign an ip (during or after creation)
polycubectl r1 ports add port1 ip=10.0.1.1/24
polycubectl r1 ports add port2 ip=10.0.2.1/24
polycubectl r1 ports add port1 ip=10.0.3.1/24
```

### Connect Ports


Two primitives are available: ``set peer`` or ``connect``.

## Set peer


The ``peer`` parameter defines where the port is connected to, it is possible to connect ports to linux netdevs or to ports belonging to other cubes.

- set peer to a ``netdev`` name in order to connect to it, (``eth0``, ``wlan0``, ``veth1``...)
- set peer to ``cube_name:port_name`` to connect the port to the port of another cube. (e.g. ``br1:port1``). In this case the peer on both ports have to be set in order to create the connection.

If the ``peer`` is empty it means the port is down, so packets are not received from it, and packets sent through it are dropped.

Following is an example, referred to previous picture.

```
#using set peer
polycubectl r1 ports port1 set peer=veth1
polycubectl r1 ports port3 set peer=eth0
polycubectl r1  ports port2 set peer=br1:port2
polycubectl br1 ports port2 set peer=r1:port2
```

## Connect


The ``connect`` primitive provides an alternative way to connect ports.

- connect to a ``netdev`` - Use ``polycubectl connect <cube1>:<port1> <netdev>``
- connect to ``cube_name:port_name`` - Use ``polycubectl connect <cube1>:<port1> <cube_name>:<port_name>``


Following is an example, referred to previous picture.

```
#using connect
polycubectl connect r1:port1 veth1
polycubectl connect r1:port3 eth0
polycubectl connect r1:port2 br1:port2
```

### Attach and Detach primitives


These primitives allow to associate transparent cubes to standard cube's ports or to netdevs on the system.

```
polycubectl attach firewall1 r1:port2

polycubectl attach firewall0 veth1
```

## Hook points


A Polycube service can be instantiated in a way that works when attached to either ``TC``, ``XDP_SKB`` or ``XDP_DRV`` eBPF hooks.
The ``XDP_DRV``, which can guarantees the best performance, can be used only if it is supported by your NIC driver.

To create a service cube and select the desired hook point, just use this command:

```

polycubectl <service name> add <cube name> type=<hook point>

```

For example if you want to create an instance of the router service working with the XDP_SKB hook point:

```

polycubectl router add r1 type=XDP_SKB

```

By default, a service is instantiated in ``TC`` mode.



## Traffic debugging with Span Mode

While the traffic flowing on a network device (e.g., veth0) can be captured and analyzed with tools such as Wireshark or tcpdump, the traffic flowing through an eBPF chain (e.g, a Polycube router connected to a Polycube bridge) is not visible outside eBPF, hence it cannot be captured by a sniffing program.

To overcome this problem, shadow cubes have a mode called **span** that allows to duplicate (hence, capture and analyze) all the traffic flowing through the ports of the cube to the corresponding virtual ethernet devices of the linked namespace.
For those who come from 'traditional' hardware networking, _span_ is the term used when you want to duplicate the traffic from a first port of a network device to a second port of the same network device, e.g., to analyze the incoming data.
Span mode is very useful for debugging; a traditional sniffing program such as Wireshark or Tcpdump can sniff all the traffic flowing through a shadow cube, selecting each port.

To activate the span mode, use the following command: ``polycubectl <cubename> set span=true``.

Note that the span mode consumes many resources when it is active, so it is disabled by default; we recommend to use it only when needed.

**Note**. Span mode duplicates traffic in the dedicated namespace, but the cube continues to handle traffic as usual.
This could create some problems when the Linux kernel is involved in the processing.
For example, if we have a shadow router with active span mode we should avoid to activate the IP forwarding on Linux, otherwise the router service forwards packets and copies them to the namespace, the namespace forwards again packets and there will be a lot of duplicated traffic.