Libraries used by Polycube
--------------------------

This folder includes all the libraries that are required by Polycube and that are statically included in the source files.

Although this makes more difficult the update of the above libraries to more recent versions, these are used for limited purposes and hence this necessity should not be so common. In exchange, this guarantees that the Polycube code always compiles with the above components.

BCC
----

From the official Githubn of  `BCC <https://github.com/iovisor/bcc>`: BPF Compiler Collection or BCC is a toolkit for creating efficient kernel tracing and manipulation programs, and includes several useful tools and examples. It makes use of extended BPF (Berkeley Packet Filters), formally known as eBPF, a new feature that was first added to Linux 3.15. Much of what BCC uses requires Linux 4.1 and above.

Jsoncons
--------

The library chosen to obtain the value of a metric from the json of an instanced cube used the jsonpath is `jsoncons <https://github.com/danielaparker/jsoncons>`_.
In brief, from the official documentation: jsoncons is a C++, header-only library for constructing JSON and JSON-like data formats such as CBOR. 

In particular we use the extension of jsonpath that implements Stefan Goessner's JSONPath. It also supports search and replace using JSONPath expressions.

All extensions except jsonpath have been deleted and documentation files, examples, tests and more that have not been used have been deleted. All this to reduce the amount of code and keep only what is actually used.

In addition, the CMakeList file has been modified to adapt to the changes made, for example the test part has been removed.

If you want to know how this library is used in Polycube, check the developer metrics in the documentation.

To compile this library you need only to run the main makefile.


polycube
---------

This library contains some common code that can be reused across multiple network functions. In facilitates the creation of links between services (i.e., to create a service chain), it provides common primitives such as a logging system, it facilitates the access to eBPF maps, and more. This library leverages [BCC](https://github.com/iovisor/bcc/) for compiling and injecting eBPF programs into the kernel.


Prometheus-cpp
---------------
The library chosen to create metrics was `Prometheus-cpp <https://github.com/jupp0r/prometheus-cpp.git>`_ .

Also for Prometheus-cpp we have tried to minimize the code and keep only what is used, deleting the documentation and other files that are not used. In particular, only the core part needed to create and manage metrics remains. The pull and push parts that have not been used have been canceled, as googletest and civitweb as submodules (3rdparty) have also been eliminated.

In addition, the CMakeList file has been modified to adapt to the changes made, for example the parts that dealt with the tests, the submodules and the push and pull parts have been deleted.

If you want to know how this library is used in Polycube, check the developer metrics in the documentation.

spdlog
------
From the official Github of `spdlog<https://github.com/gabime/spdlog>`: very fast, header-only/compiled, C++ logging library.

viface
------

From the official Github of `libviface<https://github.com/HPENetworking/libviface>`: libviface is a small C++11 library that allows to create (or hook to) and configure network interfaces in Linux based Operating Systems.

About the integration in Polycube
---------------------------------
For the bcc library is created a .so file to have the link of the shared library.

For the prometheus-cpp library we use in the install.sh file the "-DBUILD_SHARED_LIBS=ON" flag so we have the link of the shared library but if you set "-DBUILD_SHARED_LIBS=OFF" you can have a static library (.a file).

Libraries like jsoncons, polcyube, spdlog and viface are included directly.
