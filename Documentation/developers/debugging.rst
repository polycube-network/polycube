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

Logging in the data plane
*************************

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


The ``pcn_pkt_log(ctx, level)`` primitive sends a packet to the control plane where it is printed in a tcpdump-like format.
``ctx`` and ``level`` are the same as in ``pcn_log``.
This feature is only designed for developers, so final version of services should not include this.

Usage example:

::

  pcn_pkt_log(ctx, LOG_DEBUG);


.. _logging-control-plane:

Logging in the control plane
****************************

Custom ``printf()`` or similar primitives, which make the code difficult to debug, should not be used in the Polycube control plane.
In fact, the ``pcn_log()`` primitive presented below can be used also in the control plane, as Polycube includes a powerful logging system implemented within a dedicated class.

Usage example:

::

  logger()->info("Connected port {0}", port_name);
  logger()->debug("Packet size: {0}", packet.size());
  


Debug mode using IDE
^^^^^^^^^^^^^^^^^^^^^^^^^^^


VS Code 
***************


1. Usually to build Polycube, a similar thing is done but a "build" directory is used and nothing is set as DCMAKE_BUILD_TYPE. 
As explained in the answer to the question `here <https://stackoverflow.com/questions/7724569/debug-vs-release-in-cmake>`_, you need to execute the following commands (first set the appropriate breakpoints):

  :: 

    mkdir Debug
    cd Debug
    cmake -DCMAKE_BUILD_TYPE=Debug ..
    make -j

And then to install Polycube, execute:

  ::

    sudo make install

2. Create a ``launch.json`` file, following the VS Code instructions:

   1. Run
   2. Start Debugging (or Add Configuration)
   3. Select the C ++ template (GDB/LLDB)
   4. Default configuration
   5. As "program" put ``/usr/local/bin/polycubed`` or if the installation script has been modified in any way, enter the path that comes out as a result of the  ``which polycubed`` command
   6. As "args" put ``["--loglevel=DEBUG"]`` to enable Polycube debug mode logs that provide diagnostic information useful to people more than just developers
   7. Later, also following this `link <https://stackoverflow.com/questions/40033311/how-to-debug-programs-with-sudo-in-vscode>`_, create a file called "gdb" in  "home/{username}" (for example) and enter ``pkexec /usr/bin/gdb "$@"`` to make sure that you are prompted to enter the password at startup. This is because Polycube requires root permissions to run. Finally just edit the ``launch.json`` file with these 3 lines:

  ::

      "externalConsole": false,
      "miDebuggerPath": "/home/<username>/gdb",
      "MIMode": "gdb",



The final ``launch.json`` file should be something like the follow:
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
^^^^^^


There are two equivalent ways to debug Polycube with CLion:

1. Run CLion with sudo (Polycube needs root permissions)
2. Set breakpoints and build/install Polycube (as explained above)
3. At this point there are two possibilities:

   a. Use the CLion debugger: at the top right select Debug 'polycube'; or
   b. Run Polycube in a terminal like that `sudo polycubed --loglevel = DEBUG` in order to enable the Polycube Debug logs and then from CLion, go to Run->Attach to Process.. and search for "polycubed"

4. At this point, from another terminal, just use polycubectl to interact with Polycube

