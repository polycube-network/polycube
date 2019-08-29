Debugging Polycube services
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The main way to debug a service in Polycube is by using debug log messages.
Polycube provides the primitives that enables to print messages (from both data and control plane) in the **same** log file, hence facilitating the integrated debugging of both data and control plane components.
However, primitives to be used are different in the above two cases and will be presented later in this document.

By default, the log file can be found in ``/var/log/polycube/polycubed.log`` and it contains all the output produced by the polycubed daemon and the running services.


Configuring logging levels
***************************************
The Polycube framework defines the above six log levels, listed in increasing order of importance, following the general accepted practice proposed on `StackOverflow <https://stackoverflow.com/questions/2031163/when-to-use-the-different-log-levels>`_:
    - ``trace``: when the developer would "trace" the code and trying to find one part of a function specifically.
    - ``debug``: information that is diagnostically helpful to people more than just developers (IT, sysadmins, etc.).
    - ``info`` (*default value*): information that is usually useful to log (service start/stop, configuration assumptions, etc); information that users would like to have available but usually do not care about under normal circumstances.
    - ``warning``: anything that can potentially cause application oddities, but that can usually be automatically recovered by the framework, such as switching from a primary to backup server, retrying an operation, missing secondary data, etc.
    - ``error``: any error which is fatal to the operation, but not the service or application, such as cannot open a required file, missing data, etc.. These errors should force user (administrator, or direct user) intervention.
    - ``critical``: any error that is forcing a shutdown of the service or application to prevent data loss (or further data loss). These errors should be reserved only for the most heinous errors and situations where there is guaranteed to have been data corruption or loss.

Each Polycube service has a ``loglevel`` property that enables the printing of log messages filtered by the choosen level.
Setting the ``loglevel`` to ``X`` means that all the messages of that loglevel and above will be printed.
For instance, if ``loglevel=warning``, all messages marked as ``warning``, ``error`` and ``critical`` are printed.

Setting the ``loglevel`` property of a service is documented in :ref:`polycubectl section <complex-conf>`.


**Note 1**: the loglevel of ``polycubectl`` does not affect the outputs produced by the logs of individual services. In other words, ``polycubectl`` loglevel can be set to ``info``, while the loglevel of the ``bridge`` service can be set to ``trace``.

**Note 2**: the log file can be easily filtered on either (1) a specific loglevel or (2) a specific service by using the standard Unix ``grep`` tool.

    Usage examples:

    ::

        sudo polycubed | grep "debug"
        sudo polycubed | grep -E "warning|error|critical"
        sudo polycubed | grep "bridge"

.. _logging-in-the-data-plane:

Logging in the data plane
***************************************
In order to avoid the use of the ``bpf_trace_printk()`` primitive (which is not integrated with control plane logging and should not be used in Polycube), the Polycube framework provides the ``pcn_log()`` primitive.

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
`ctx` and `level` are the same as in `pcn_log`.
This feature is only designed for developers, so final version of services should not include this.

`polycube-tools <https://github.com/mauriciovasquezbernal/polycube-tools>`_ must be installed and ``polycube`` cmake must be configured with `cmake -DHAVE_POLYCUBE_TOOLS=ON ..`

Usage example:

::

  pcn_pkt_log(ctx, LOG_DEBUG);

.. _logging-in-the-control-plane:

Logging in the control plane
******************************************

In order to avoid the use of custom ``printf()`` or similar primitives, which makes the code difficult to debug, Polycube includes a logging system with a proper class.

Usage example:

::

  logger()->info("Connected port {0}", port_name);
  logger()->debug("Packet size: {0}", packet.size());
  
