Router developer
================

**TODO**: this document has to be updated!

Data plane - fast path
----------------------

This flowchart summarizes the fast path algorithm of the router, which is implemented in [src/Router_dp.h](./src/Router_dp.h).

TODO: should we include this image?

![eBPF flowchart](./docs/datapath.png)

Data plane - slow path
----------------------

This flowchart summarizes the slow path algorithm of the router, which is implemented in [src/Router.cpp](./src/Router.cpp).

TODO: what about this?

![Slowpath flowchart](./docs/slowpath.png)