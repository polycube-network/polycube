Helloworld
==========

This service is oriented to demonstrate to developers how to create a minimal cube, which includes both the dataplane `fast path` and the control/management `slow path`.

Helloworld is a simple service that receives the traffic on a network interface and can either:

- send packets to a second interface
- send packets to the slow path
- drop packets

The behavior of this service can be changed by setting the ``action`` flag, which tells the data plane how the packets have to be processed.

How to use
----------

::

    # create network namespaces with usual commands
    #   we assume you have two interfaces (veth1 
    #   and veth2 already working here)


    # create the instance
    polycubectl helloworld add hw0

    # add ports (only two are supported)
    polycubectl hw0 ports add port1 peer=veth1
    polycubectl hw0 ports add port2 peer=veth2

    # change action
    polycubectl hw0 set action=drop

    # send packets to the service

    # try another action, such as forward
    polycubectl hw0 set action=forward


Service structure
-----------------

Helloworld includes the minimum amount of code that a service requires to be run.

- **src/helloworld_dp.h** contains the eBPF code of the cube. It is not necessary to define the code in a separated file, however we suggest it to keep things organized.
- **src/helloworld.[h,cpp]** contain the definition of the main class of the service. This class represents an instance of the service and implements the methods for creating and destroying cubes, adding and removing ports from it. Finally, it implements the control plane that allows to change the behavior of the cube by updating values in the eBPF maps.
- **src/api** contains the rest api implementation, these files are automatically generated and do not have to be modified by the programmer.
- **src/[default-src, interface, serializer]** contain different pieces of the services that do not need to be modified by the developer.
- **src/helloworld-lib.cpp** contains the implementation of interface that is used when the service is compiled as a shared library.
- **datamodel/helloworld.yang** contains the service datamodel.


Compile and install
-------------------

By default services shipped with `polycube` are compiled and installed all together.

The instructions below are provided for people who want to compile and install this service separately, which may be useful in case we want to create our own service and we start from HelloWorld as minimal skeleton.

In order to compile and install a service run the following commands:

::

    # in the service folder
    mkdir build; cd build
    cmake ..
    make
    sudo make install


The shared library implementation is installed in the default libraries path of the host.
