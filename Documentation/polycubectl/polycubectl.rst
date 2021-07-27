polycubectl: the command-line interface for Polycube
====================================================

``polycubectl`` is the Command Line Interface (CLI) for Polycube.

``polycubectl`` is a generic CLI, that enables the user to interact with ``Cubes`` (``bridge``, ``router``, ...) and with some framework primitives to ``connect``, ``show`` and build complex ``topologies``.

``polycubectl`` does not need to be modified when a new cube is developed ad added to Polycube. Its service-agnostic nature, thanks to the use of YANG data models, enables the CLI to be service-independent.


Install
-------

``polycubectl`` is installed by default with ``Polycube``.
Refer to :doc:`quickstart <../quickstart>` or general :doc:`install <../installation>` guide for more info.

How to use
----------

**NOTE**: ``polycubed`` must be running in order to use ``polycubectl``.
You can start the daemon typing ``sudo polycubed`` in another terminal.
Refer to :doc:`Quick Start <../quickstart>`.


::

        # Show help
        polycubectl --help
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
        packetcapture     service   Packetcapture Service

        connect           command   Connect ports
        disconnect        command   Disconnect ports

        attach            command   Attach transparent cubes
        detach            command   Detach transparent cubes

        services          command   Show/Add/Del services (e.g. Bridge, Router, ..)
        cubes             command   Show running service instances (e.g. br1, nat2, ..)
        topology          command   Show topology of service instances
        netdevs           command   Show net devices available

The general syntax for ``polycubectl`` is the following:

::

        polycubectl [parent] [command] [child] [argument0=value0] [argument1=value1]


- ``parent``: path of the parent resource where the command has to be applied.
- ``command``: ``add``, ``del``, ``show``, ``set`` or a yang action.
- ``child``: specific resource where the command is applied.
- ``argument``: some commands accept additional commands that are sent in the body request.

Some examples:

::

        polycubectl router r0 add loglevel=debug
        polycubectl r0 ports add port1 ip=10.1.0.1 netmask=255.255.0.0
        polycubectl r0 show routing table
        polycubectl r0 ports port1 set peer=veth1
        polycubectl r0 ports del port1

        # yang action
        polycubectl firewall1 chain ingress append src=10.0.0.1 action=DROP


The best way to know what is the syntax for each service is to use the `Help`_ or the bash completion by pressing ``<TAB>`` at any point.


``polycubectl`` is also able to read the contents of the request from the standard input, it can be used in two ways:

.. _complex-conf:

Passing complex configuration from the command line
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
::

        # create a helloworld instance with loglevel debug and action forward
        polycubectl helloworld add hw0 << EOF
        {
        "loglevel": "debug",
        "action": "forward"
        }
        EOF


Reading configuration from a file
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
::

        # create helloworld from a yaml file
        polycubectl helloworld add hw0 < hw0.yaml

        # create a router from a json file
        polycubectl router add r0 < r0.json

        # add a list of cubes
        polycubectl cubes add < mycubes.yaml

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

By default, ``polycubectl`` contacts ``polycubed`` at ``http://localhost:9000/polycube/v1/``.
The user can override this configuration with following instructions.

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
