# K8lbrp


This service implements a ``Reverse Proxy Load Balancer``.
It is inspired by the functioning of the already existing `pcn-loadbalancer-rp` service but extends its architecture
in order to increase the number of supported ports. This service is specifically designed as part of our kubernetes
networking solution (please see [polykube](https://github.com/polycube-network/polykube) to get more information about
it).

The purpose of this service is to perform the necessary translation of the Kubernetes Service’s associated IP and port
with the ones of a real Service’s backend Pod, according to the load balancing logic. A k8slbrp port can be a FRONTEND
or a BACKEND port. Depending on the port mode, the k8slbrp can support one or more frontend ports. Two port modes are
supported: SINGLE and MULTI. In SINGLE port mode, the load balancer supports only a single FRONTEND port; in MULTI
port mode, multiple FRONTEND ports are supported. In both cases, only a BACKEND port is supported. The SINGLE port
mode is designed to allow the k8slbrp to work exactly as a `pcn-loadbalancer-rp` instance.

Regardless the port mode, if a packet coming from a FRONTEND port is directed to a Service, DNAT is performed on it;
the corresponding SNAT is performed for packets coming from backends and on the way back to clients. Like in the case
of the pcn-lbrp service, packet are hashed to determine which is the correct backend; all packets belonging to the same
TCP/UDP session will always be terminated to the same backend server. The translation performed is from ``vip:vport``
to ``realip:realport``.


## Features


- Support for different port mode (SINGLE and MULTI)
- Support for multiple frontend port (in MULTI port mode)
- Support for multiple virtual services (with multiple ``vip:protocol:vport`` tuples)
- Support for ICMP Echo Request, hence enabling to ``ping`` virtual servers
- Session affinity: a TCP session is always terminated to the same backend server even in case the number of backend servers changes at run-time (e.g., a new backend is added)
- Each virtual service can be mapped to a different number of backend servers
- Mapping can be differentiated per protocol (e.g., some backend are dedicated to serve TCP traffic, other UDP, etc)
- Traffic is forwarded to backend services after performing an IP address rewriting in the packet (from ``vip:vport`` to the selected ``realip:realport``); hence, the ``vip`` virtual IP address and the IP address of the actual servers should belong to different IP networks
- Support for weighted backends

## Limitations


- In SINGLE port mode, only two ports are supported (a FRONTEND and a BACKEND port)
- In MULTI port mode, multiple FRONTEND port are supported but only a single BACKEND port can exists
- In MULTI port mode, an IPv4 address must be configured on FRONTEND port creation in order to allow packets to
flow back to the frontend clients
- In MULTI port mode, the only supported topology is the one leveraged in the
[polykube](https://github.com/polycube-network/polykube) kubernetes networking solution

