# Load Balancer (RP)


This service implements a ``Reverse Proxy Load Balancer``.
According to the algorithm, incoming IP packets are delivered to the real servers by replacing their IP destination address with the one of the real server, chosen by the load balancing logic. Hence, IP address rewriting is performed in both directions, for traffic coming from the Internet and the reverse. Packet are hashed to determine which is the correct backend; the hashing function guarantees that all packets belonging to the same TCP/UDP session will always be terminated to the same backend server.

This service supports two different port types (FRONTEND and BACKEND) and two different port modes (SINGLE and MULTI). Depending on the port mode, the cube can have one or more FRONTEND ports: in SINGLE port mode (which is the default one), only a single FRONTEND port is supported, whereas multiple FRONTEND ports are supported in MULTI port mode. The MULTI port mode is specifically designed in order to allow the service to work properly as part of our Kubernetes networking solution (please see [polykube](https://github.com/polycube-network/polykube) to get more information about it). Regardless the port mode, only a BACKEND port is supported.

If a packet coming from a FRONTEND port is directed to a service, DNAT is performed on it; the corresponding reverse natting operation is performed for packets coming from backends and on the way back to clients. ARP packets are forwarded as they are (in MULTI port mode, if the packet is an ARP request from the BACKEND port, it is forwarded to the right FRONTEND port). Unknown packets (e.g., IPv6) are simply forwarded as they are (in MULTI port mode, if the packet comes from the BACKEND port, it is flooded to all the FRONTEND ports).

## Features


- Support for different port modes (SINGLE and MULTI)
- Support for multiple frontend ports (in MULTI port mode)
- Support for multiple virtual services (with multiple ``vip:protocol:port`` tuples)
- Support for ICMP Echo Request, hence enabling to ``ping`` virtual servers
- Session affinity: a TCP session is always terminated to the same backend server even in case the number of backend servers changes at run-time (e.g., a new backend is added)
- Each virtual service can be mapped to a different number of backend servers
- Mapping can be differentiated per protocol (e.g., some backend are dedicated to serve TCP traffic, other UDP, etc)
- Traffic is forwarded to backend services after performing an IP address rewriting in the packet (from ``vip:port`` to the selected ``realip:port``); hence, the ``vip`` virtual IP address and the IP address of the actual servers should belong to different IP networks
- Support for weighted backends (more later)

## Limitations

- In SINGLE port mode, only two ports are supported (a FRONTEND and a BACKEND port)
- In MULTI port mode, multiple FRONTEND port are supported but only a single BACKEND port can exists
- In MULTI port mode, an IPv4 address must be configured on FRONTEND port creation in order to allow packets to flow back to the frontend clients
- In MULTI port mode, the supported topology is the one leveraged in the [polykube](https://github.com/polycube-network/polykube) Kubernetes networking solution


## How to use


### Weighted backends


Each backend supports a ``weight`` that determines how incoming sessions are distributed across backends; for example, a backend with ``weight = 10`` will receive (in average) twice the sessions of another backend with ``weight = 5``.


## Deployment


A set of ``virtual services``, which are specified by a Virtual IP address, protocol and a port (``vip:protocol:port``), are mapped to a given set of ``backend services``, actually running on multiple real servers.

Hence, in SINGLE port mode, this service exports two network interfaces:
- Frontend port: connects the LB to the clients that connect to the virtual service, likely running on the public Internet
- Backend port: connects the LB to to backend servers

```

                                +---------------+
[outside network]     (port 0) | loadbalancer  | (port 1)     [inside network]
[public internet]  <-----------| reverse proxy | ---------->  [backend services]
                        (vip)  +---------------+
```

## Examples


A possible configuration example is available in the ``examples/``.
