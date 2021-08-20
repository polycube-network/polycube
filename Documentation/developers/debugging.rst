Debugging Polycube services
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The main way to debug a service in Polycube is by using debug log messages.
Polycube provides the primitives that enables to print messages (from both data and control plane) in the **same** log file, hence facilitating the integrated debugging of both data and control plane components.
However, primitives to be used are different in the above two cases and will be presented later in this document.

By default, the log file can be found in ``/var/log/polycube/polycubed.log`` and it contains all the output produced by the polycubed daemon and the running services.


Configuring logging levels
**************************

Polycube defines six log levels, listed below in increasing order of importance, following the general accepted practice proposed on `StackOverflow <https://stackoverflow.com/questions/2031163/when-to-use-the-different-log-levels>`_:

  - ``trace``: when the developer would "trace" the code and trying to find one part of a function specifically.
  - ``debug``: information that is diagnostically helpful to people more than just developers (IT, sysadmins, etc.).
  - ``info`` (*default value*): information that is usually useful to log (service start/stop, configuration assumptions, etc); information that users would like to have available but usually do not care about under normal circumstances.
  - ``warning``: anything that can potentially cause application oddities, but that can usually be automatically recovered by the framework, such as switching from a primary to backup server, retrying an operation, missing secondary data, etc.
  - ``error``: any error which is fatal to the operation, but not the service or application, such as cannot open a required file, missing data, etc.. These errors should force user (administrator, or direct user) intervention.
  - ``critical``: any error that is forcing a shutdown of the service or application to prevent data loss (or further data loss). These errors should be reserved only for the most heinous errors and situations where there is guaranteed to have been data corruption or loss.

Each Polycube service has a ``loglevel`` property that enables the printing of log messages filtered by the choosen level.
Setting the ``loglevel`` to ``X`` means that all the messages of that loglevel and above will be printed.
For instance, if ``loglevel=warning``, all messages marked as ``warning``, ``error`` and ``critical`` are printed.

Setting the ``loglevel`` property of a service can be achieved by creating the service with the proper property, such as in the following:

::

        polycubectl router r0 add loglevel=debug

This creates a new instance of a router, called ``r0``, with the highest logging level (``debug``) turned on.


**Note 1** The loglevel of each individual service does not affect the loglevel of the ``polycubectl`` command line. In other words, ``polycubectl`` logging can be set to minimal (i.e., logging level = ``info``), while a given service can have the logging level equal to ``debug``.
More information is available in the :ref:`configuring polycubectl <polycubectl-configuration>` section.

**Note 2** The loglevel of each individual service does not affect the loglevel of the ``polycubed`` system daemon. In other words, ``polycubed`` logging can be set to minimal (i.e., logging level = ``info``), while a given service can have the logging level equal to ``debug``.
More information is available in the :ref:`debugging the polycubed daemon <polycubed-debugging>` section.

**Note 3**: the log file can be easily filtered on either (1) a specific loglevel or (2) a specific service by using the standard Unix ``grep`` tool.

    Usage examples:

    ::

        sudo polycubed | grep "debug"
        sudo polycubed | grep -E "warning|error|critical"
        sudo polycubed | grep "bridge"



.. _logging-data-plane:

Logging the eBPF data plane
***************************

The common eBPF primitive ``bpf_trace_printk()``, which is widely used in other eBPF frameworks, should not be used in Polycube.
Polycube offers a new ``pcn_log()`` primitive, which integrates both data and control plane logging in the same location.

Syntax:

::

    pcn_log(ctx, level, msg, args...)

  - ``ctx``: packet being processed.
  - ``level``: type of logging level: ``LOG_TRACE``, ``LOG_DEBUG``, ``LOG_INFO``, ``LOG_WARN``, ``LOG_ERR``, ``LOG_CRITICAL``.
  - ``msg``: message to print
  - ``args``: arguments of the message

Three special format specifiers are available:

  - ``%I``: print an IP address
  - ``%M``: print a MAC address
  - ``%P``: print a TCP/UDP port

Please note the the custom specifiers expect the data to be in *network byte order* while standard specifiers expects it to be in *host byte order*.

Current limitations of ``pcn_log()``:

  - Cannot be used inside a macro
  - Maximum 4 arguments are allowed

Usage example:

::

  pcn_log(ctx, LOG_DEBUG, "Receiving packet from port %d", md->in_port);



.. _logging-control-plane:


Logging the control plane
*************************

Custom ``printf()`` or similar primitives, which make the code difficult to debug, should not be used in the Polycube control plane.
In fact, the ``pcn_log()`` primitive presented below can be used also in the control plane, as Polycube includes a powerful logging system implemented within a dedicated class.

Usage example:

::

  logger()->info("Connected port {0}", port_name);
  logger()->debug("Packet size: {0}", packet.size());
  


Debugging using Integrated Development Environment (IDE)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


VS Code 
***************

Polycube uses ``cmake`` to create the compilation environment (Makefiles, etc), which, by default is configured to create the executables in ``Release`` mode.
To debug your code, you must compile Polycube in ``Debug`` mode, with the following commands (follow also `here <https://stackoverflow.com/questions/7724569/debug-vs-release-in-cmake>`_):

  :: 

    mkdir Debug
    cd Debug
    cmake -DCMAKE_BUILD_TYPE=Debug ..
    make -j

Once all the Polycube executables have been created with _debug_ information, you must install them in the default folders with the following command:

  ::

    sudo make install

Now, let's create a ``launch.json`` file, following the VS Code instructions:

   1. Run
   2. Start Debugging (or Add Configuration)
   3. Select the C ++ template (GDB/LLDB)
   4. Default configuration
   5. As "program", add ``/usr/local/bin/polycubed``, which represents the default location. Alternatively, if you modified the installation script, enter the path that comes out as a result of the  ``which polycubed`` command
   6. As "args", add ``["--loglevel=DEBUG"]`` to enable Polycube debug mode logging, which provides diagnostic information useful to people more than just developers. Note that the Polycube _debug_ logging (which simply means _the highlest level of verbosity_) is independent from the build type, hence it has to be enabled explicitly.
   7. Later, also following this `link <https://stackoverflow.com/questions/40033311/how-to-debug-programs-with-sudo-in-vscode>`_, create a file called "gdb" in  "home/{username}" (for example) and enter ``pkexec /usr/bin/gdb "$@"`` to make sure that you are prompted to enter the password at startup. This is because Polycube requires root permissions to run. Finally just edit the ``launch.json`` file with these 3 lines:

  ::

      "externalConsole": false,
      "miDebuggerPath": "/home/<username>/gdb",
      "MIMode": "gdb",


The final ``launch.json`` file should be something like the following:
::

    {
        // Use IntelliSense to learn about possible attributes.
        // Hover to view descriptions of existing attributes.
        // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
        "version": "0.2.0",
        "configurations": [
            {
                "name": "(gdb) Launch",
                "type": "cppdbg",
                "request": "launch",
                "program": "/usr/local/bin/polycubed",
                "args": ["--loglevel=DEBUG"],
                "stopAtEntry": false,
                "cwd": "${fileDirname}",
                "environment": [],
                "externalConsole": false,
                "miDebuggerPath": "/home/pino/gdb",
                "MIMode": "gdb",
                "setupCommands": [
                    {
                        "description": "Abilita la riformattazione per gdb",
                        "text": "-enable-pretty-printing",
                        "ignoreFailures": true
                    }
                ]
            }
        ]
    }



CLion
******

If you use CLion, you can debug Polycube with the following steps:

1. Run CLion with sudo (Polycube needs root permissions)
2. Set breakpoints and build/install Polycube (as explained above)
3. At this point there are two equivalent ways to debug Polycube with CLion:

   a. Use the CLion debugger: at the top right select Debug 'polycube';
   b. (or) Run Polycube in a terminal with enabled debug logs (e.g., ``sudo polycubed --loglevel = DEBUG``); then, from CLion, go to Run->Attach to Process.. and search for "polycubed"

4. At this point, from another terminal, just use ``polycubectl`` to interact with Polycube.



Debugging network traffic using tcpdump/Wireshark
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Debugging (i.e., sniffing) network traffic frlowing *between* two Polycube services (e.g., on the link that connects a Polycube router to a Polycube bridge) can be achieved with the :ref:`span mode <span-mode>`.
Please refer to that section for more details and for some working examples.


Let's see how to capture packets using the concepts of shadow port and :ref:`span mode <span-mode>` to see what happens. 
The topology used is a simpler version of the shadow type test3 of the router service, in order to understand well the various components, how to use tcpdump/Wireshark and understand what happens.


::             	

			pcn-r1 (network namespace)
		      +-----------------------------+
   +-----------+      |          +------+           |	   +-----------+
   |  [veth1]  | ------ to_veth1-|  r1  |-to_veth2  ------ |  [veth2]  | 
   +-----------+      |	         +------+           |	   +-----------+
       ns1 	      +-----------------------------+		ns2
  

Topology construction
**********************

First we create the namespaces, we add the veths that are connected in an appropriate way and on each of them a network is configured and finally the default gateway is also added.


::

  for i in `seq 1 2`;
	do
		sudo ip netns add ns${i}
		sudo ip link add veth${i}_ type veth peer name veth${i}
		sudo ip link set veth${i}_ netns ns${i}
		sudo ip netns exec ns${i} ip link set dev veth${i}_ up
		sudo ip link set dev veth${i} up
		sudo ip netns exec ns${i} ifconfig veth${i}_ 10.0.${i}.1/24
		sudo ip netns exec ns${i} route add default gw 10.0.${i}.254 veth${i}_
	done


In another terminal just run polycube using ``sudo polycubed``.
Regarding the concept of shadow: a shadow cube (only a standard cube can be shadow) is associated with a Linux network namespace. 
The parameters between the shadow cube and the namespace are aligned. 
A port defined on a shadow cube is also visible from the network namespace:

- the user can decide to configure the ports using Linux (e.g. ifconfig or the ip command) or polycubectl; for example: ``polycubectl <cubename> ports <PortName> set ip=<IpAddress>`` it is the same as ``ip netns exec pcn-<cubename> ifconfig <PortName> <IpAddress>``.
- the developer can let Linux handle some traffic by sending it to the namespace (e.g. ARP, ICMP, but in general all those protocols able to be managed by a tool running inside the namespace);

So, let's create the r1 router:


::

	polycubectl router add r1 shadow=true  
	

Now let's add to the router r1 a port for each veth (to_vethX) with an appropriate IP address:


::

	for i in `seq 1 2`;
	do
		polycubectl router r1 ports add to_veth${i} ip=10.0.${i}.254/24
		polycubectl router r1 ports to_veth${i} set peer=veth${i}
	done



As for the span mode, when it is activated it shows all the traffic seen by the service also at the namespace.

::

	polycubectl router r1 set span=true


To see what has just been created you can use these following commands:

* ``sudo ip netns``: from the output you can see three namespaces created, ns1, ns2 and in particular pcn-r1 which is created specifically for the shadow cube;
* ``sudo ip netns exec ns1 ip a`` (equivalent for ns2): from the output it is possible the network interfaces of the namespace ns1, in this case ``veth1``, with the assigned ip address and other information;
* ``ip a``: as before but in this case we see the network interfaces of the default namespace, in particular ``r1-to_veth1`` and ``r1-to_veth2``





If you want to see the topology, just use ``polycubectl topology``, it should show something like this:

::

	loglevel  name  service-name  shadow  span  type    name (ports)      peer (ports)
	INFO      r1    router        true    true  TC      ns_port_to_veth1  r1-to_veth1
                                                     	    ns_port_to_veth2  r1-to_veth2
                                                      	    to_veth1          veth1
                                                      	    to_veth2          veth2

Or, if you want to see information about r1 ports, just use ``polycubectl r1 ports show`` and it should show something like this:

::

  ip             mac                name      peer   status  uuid
  10.0.1.254/24  ba:e7:da:ec:49:17  to_veth1  veth1  UP      3e92bd2f-567d-4adc-b7c1-820eb9cca9a7
  10.0.2.254/24  46:f3:b4:48:2e:7f  to_veth2  veth2  UP      b51d21f3-366a-4f39-b3af-58e07cf62c44



tcpdump/Wireshark
*****************

Now, let's use Wireshark/tcpdump on the interface we want.

For tcpdump we only need to do ``tcpdump -ni <interface-name>``. For example, if we want to see the traffic passing through router r1's ``to_veth1`` and we only want ICMP traffic, we need to give this command:

::

	sudo tcpdump icmp -ni r1-to_veth1


As for using Wireshark, just open Wireshark and select the network interface to listen to.



Sending traffic
***************

Now we can ping from ns1 to ns2 and for example we want to send only 4 packets, so:

::

	sudo ip netns exec ns1 ping 10.0.2.1 -c 4


The output of the ping command should be something like that (yes there are DUP packets):

::

    PING 10.0.2.1 (10.0.2.1) 56(84) bytes of data.
    64 bytes from 10.0.2.1: icmp_seq=1 ttl=63 time=0.244 ms
    64 bytes from 10.0.2.1: icmp_seq=1 ttl=63 time=0.437 ms (DUP!)
    64 bytes from 10.0.2.1: icmp_seq=1 ttl=64 time=0.582 ms (DUP!)
    64 bytes from 10.0.2.1: icmp_seq=1 ttl=64 time=0.622 ms (DUP!)
    64 bytes from 10.0.2.1: icmp_seq=2 ttl=63 time=0.058 ms
    64 bytes from 10.0.2.1: icmp_seq=2 ttl=63 time=0.127 ms (DUP!)
    64 bytes from 10.0.2.1: icmp_seq=2 ttl=63 time=0.162 ms (DUP!)
    64 bytes from 10.0.2.1: icmp_seq=2 ttl=63 time=0.198 ms (DUP!)
    64 bytes from 10.0.2.1: icmp_seq=3 ttl=63 time=0.056 ms
    64 bytes from 10.0.2.1: icmp_seq=3 ttl=63 time=0.141 ms (DUP!)
    64 bytes from 10.0.2.1: icmp_seq=3 ttl=63 time=0.179 ms (DUP!)
    64 bytes from 10.0.2.1: icmp_seq=3 ttl=63 time=0.227 ms (DUP!)
    64 bytes from 10.0.2.1: icmp_seq=4 ttl=63 time=0.135 ms

    --- 10.0.2.1 ping statistics ---
    4 packets transmitted, 4 received, +9 duplicates, 0% packet loss, time 3061ms
    rtt min/avg/max/mdev = 0.056/0.243/0.622/0.178 ms


In the tcpdump terminal, you should see something like:

::

    $ sudo tcpdump icmp -ni r1-to_veth1 
    tcpdump: verbose output suppressed, use -v or -vv for full protocol decode
    listening on r1-to_veth1, link-type EN10MB (Ethernet), capture size 262144 bytes
    17:11:11.758249 IP 10.0.1.1 > 10.0.2.1: ICMP echo request, id 12128, seq 1, length 64
    17:11:11.758346 IP 10.0.2.1 > 10.0.1.1: ICMP echo reply, id 12128, seq 1, length 64
    17:11:11.758539 IP 10.0.2.1 > 10.0.1.1: ICMP echo reply, id 12128, seq 1, length 64
    17:11:12.771174 IP 10.0.1.1 > 10.0.2.1: ICMP echo request, id 12128, seq 2, length 64
    17:11:12.771242 IP 10.0.2.1 > 10.0.1.1: ICMP echo reply, id 12128, seq 2, length 64
    ...
	


You can check also the Polycube terminal where there are also other informations about the traffic, for example the ARP traffic.



