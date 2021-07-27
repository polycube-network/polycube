Tutorial 4: Attach multiple transparent cubes
=============================================================

This simple tutorial aims to show how multiple cubes can be attached to the same interface/port. For the sake of simplicity the configuration presented in this tutorial is really simple, but enough to understand the principle behind these operations.

Before to start, remind that not only different transparent cubes (such as firewall, nat, etc.) can be attached in pipeline to the same interface, but also cubes of the same type can be inserted, like two firewalls.

The transparent cubes used as example in this tutorial are two different instances of :doc:`description <../../services/pcn-dynmon/dynmon>`, which will be attached to a :doc:`description <../../services/pcn-simplebridge/simplebridge>` port as in figure.

::

    +--------+                                      +--------+   
    +        |                  +---------+---------+        |   
 ---| bridge |------------------| dynmon0 | dynmon1 | bridge |---
    +   1    |\                  +---------+--------+   2    |   
    +--------+ \                                   /+--------+   
               |                                  /              
               |                                  |              
             br1:port1                         br2:port2         

Set up bridges
--------------

To set up the two bridges, please refer to this :doc:`description <../../services/pcn-simplebridge/example1>`, which describes how to configure the cubes and create a link between two ports.


Step1: Create transparent cubes
-------------------------------

Now that the basic topology is ready, create the two ``dynmon`` instances:

::

	polycubectl dynmon add monitorA
	polycubectl dynmon add monitorB


Step2: Attach cubes
-------------------

To completely understand all the possibility the framework offers to attach cubes, the command line helper can be used.
In bash (in many shells may not be configured by default), type the following command to see all the possible parameters:

::

	~$ polycubectl attach ?

	keyword         type                 description
	<cube>          cube                 Transparent cube to be attached (E.g. nat1)
	<port>          cube:port or netdev  Port of a cube or netdev to attach the transparent cube (E.g. rt2:port2 or eth0)
	position=value                       Position to place the cube. auto, first, last
	before=value                         Place the cube before another one
 	after=value                          Place the cube after another one

	Example:
	 attach nat1 rt:port1

As in our case, when more than one cube need to be attached in pipeline to the same interface, one parameter between position/before/after should be specified, otherwise the framework cannot understand where the user wants that cube to be placed.

Firstly, to attach *monitor1* to *br2:port2*:

::

	~$ polycubectl attach monitor1 br2:port2

Notice that since there are not other cubes attached yet, the position can be omitted. 

Finally, to attach also *monitor1* to *monitor2*:

::

	~$ polycubectl attach monitor2 br2:port2 after=monitor1
	#
	# OR
	#
	~$ polycubectl attach monitor2 br2:port2 position=last

Now the configuration is completed. Every ``dynmon`` instance is working standalone and each one can be configured differently.

**Last consideration:**

Pay attention to the order, especially when different services which can eventually decide to drop packets are working in pipeline, since the final result may not be as expected (if ``dynmon0`` decided to drop a packet, ``dynmon1`` would not receive it).
