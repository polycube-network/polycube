.. _polycubectl:

polycubectl: The Command Line Interface for Polycube
====================================================

``polycubectl`` is the Command Line Interface (CLI) for Polycube.

Install
-------

``polycubectl`` is installed by default with ``Polycube``.
Refer to :doc:`quickstart <../quickstart>` or general :doc:`install <../installation>` guide for more info.

How to use
----------

**NOTE**: ``polycubed`` must be running, in order to use ``polycubectl``.
You can start the daemon typing ``sudo polycubed`` in another terminal. Refer to :doc:`Quick Start <../quickstart>`.

``polycubectl`` is a generic CLI, that enables the user to interact with ``Cubes`` (``bridge``, ``router``, ...) and with some framework primitives to ``connect``, ``show`` and build complex ``topologies``.

::

        # Show help
        polycubectl --help

::

        Keyword           Type      Description
        simpleforwarder   service   Simple Forwarder Base Service
        simplebridge      service   Simple L2 Bridge Service
        router            service   Router Service
        iptables          service   Iptables-clone Service
        helloworld        service   Helloworld Service
        ddosmitigator     service   DDoS Mitigator Service
        firewall          service   Firewall Service
        k8switch          service   Kubernetes HyperSwitch Service
        k8sfilter         service   Kubernetes Traffic Filtering Service
        lbdsr             service   LoadBalancer Direct Server Return Service
        lbrp              service   LoadBalancer Reverse-Proxy Service
        pbforwarder       service   Policy-Based Forwarder Service
        bridge            service   Bridge Service
        nat               service   NAT Service

        connect           command   Connect ports
        disconnect        command   Disconnect ports

        services          command   Show/Add/Del services (e.g. Bridge, Router, ..)
        cubes             command   Show running service instances (e.g. br1, nat2, ..)
        topology          command   Show topology of service instances
        netdevs           command   Show net devices available


Help
^^^^

| ``polycubectl`` provides an interactive way to navigate help.
| At each depth level the user can type ``?`` to get contextual help information.
| The output of help command is basically a list of ``keywords`` that could be used (instead of ``?``), and is some case a list of ``parameters``.
| Following is an example of a possible interaction with help, to configure a ``router``.

::

        polycubectl router ?

        Keyword  Type      Description
        add      command   Add entry to a list
        del      command   Delete entry of a list
        show     command   Show entry or list [-normal | -brief | -verbose | -json | -yaml]
        <name>   string    Name of the router service

::

        polycubectl router add ?

        Keyword             Type     Description
        <name>              string   Name of the router service

        Other parameters:
        loglevel=value      string   Defines the logging level of a service instance, from none (OFF) to the most verbose (TRACE)

        Example:
        polycubectl router add router1 loglevel=INFO

::

        polycubectl router add r1


Flags
^^^^^

The ``show`` command supports the ``-normal``, ``-brief``, ``-verbose``, ``-json``, ``-yaml`` flags that affects how the output is printed on the terminal.

-hide
*****

The ``-hide=arg0[arg1[,arg2...]]`` flag allows to hide some elements in the output.
It expects a list paths to resources to be hidden.

Examples:

::

    # hide ports from output
    polycubectl router r0 show -hide=ports

    # hide uuid of ports
    polycubectl router r0 show -hide=ports.uuid

    # hide uuids and mac ports
    polycubectl router r0 show -hide=ports.uuid,ports.mac

Tutorials
^^^^^^^^^

More complete examples are available in :doc:`tutorials <../tutorials/index>`.

.. _polycubectl-configuration:

Configuration
-------------

By default, the CLI contacts ``polycubed`` daemon at ``http://localhost:9000/polycube/v1/``. The user can override this configuration with following instructions.

Parameters
^^^^^^^^^^

- ``debug`` shows HTTP requests and response sent and received by CLI
- ``expert`` enables the possibility to add new services at runtime
- ``url`` is the URL to contact polycubed
- ``cert`` client certificate when using https
- ``key`` client private key
- ``cacert`` certification authority certificate that signed the server's certificate

Configuration file
^^^^^^^^^^^^^^^^^^

Configuration file is placed at ``$HOME/.config/polycube/polycubectl_config.yaml``.

::

        # debug: shows http method/url and body of the response
        # expert: enables the possibility to add new services
        # url: is the base URL to contact the rest server

        debug: false
        expert: true
        url: http://localhost:9000/polycube/v1/
        key: ""
        cacert: ""
        cert: ""


Environment variables
^^^^^^^^^^^^^^^^^^^^^

Following are available ENV variables:

::

        POLYCUBECTL_DEBUG
        POLYCUBECTL_URL
        POLYCUBECTL_EXPERT
        POLYCUBECTL_CERT
        POLYCUBECTL_KEY
        POLYCUBECTL_CACERT

A possible example of configuration is:
::

        $ export POLYCUBECTL_URL="http://10.0.0.1:9000/polycube/v1/"
