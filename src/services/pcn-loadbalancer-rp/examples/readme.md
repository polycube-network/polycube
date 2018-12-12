# Load Balancer Service example

This example creates a complex topology in which a first namespace hosts a client that connects to three backend services hosted in other three namespaces.
The topology feature a load balancer, a router (which enables routing between namespaces) and a bridge (as the three backend namespaces share the same IP network).

The overall network topology, with the most important parameters such as IP addresses, is shown below.

```
                     +----------------+
                     | ns1 (client)   |
                     | IP: 10.0.0.1   |
                     | DG: 10.0.0.254 |
                     +----------------+
                              |
                              |
                  +------------------------+
                  |      Load Balancer     |
                  | Vservice: 13.13.13.13  |
                  | Backends: 10.0.1.2-3-4 |
                  +------------------------+
                              |
                              |
                     +-----------------+
                     |      router     |
                     | IP1: 10.0.0.254 |
                     | IP2: 10.0.1.254 |
                     +-----------------+
                              |
                              |
                  +------------------------+
                  |         bridge         |
                  +------------------------+
                  /           |            \
                 /            |             \
 +----------------+   +----------------+   +----------------+
 | ns2 (backend)  |   | ns3 (backend)  |   | ns4 (backend)  |
 | IP: 10.0.1.2   |   | IP: 10.0.1.3   |   | IP: 10.0.1.4   |
 | DG: 10.0.1.254 |   | DG: 10.0.1.254 |   | DG: 10.0.1.254 |
 +----------------+   +----------------+   +----------------+

```

In this example the client is a simple `curl`, when we want to retrieve a web page from one of the backends, or a `ping` when we want to test the reachability of the backend servers.
The server is a simple HTTP server waiting for incoming requests.

This example shows that the client can either contact directly each individual server (e.g., `ping 10.0.1.2`), or the virtual service (e.g., `ping 13.13.13.13`); in the first case the load balancer is completely transparent to the traffic, while in the second case it replaces the IP destination address of incoming packets with one of the real servers for outbound traffic, while it does the reverse for the return traffic.

Detailed commands for setting up this environment and to test that it works are provided in the script [ex1.sh](ex1.sh), available in this folder.
