polycubed: the Polycube System Daemon
=====================================

The Polycube system daemon (polycubed) is in charge of managing the lifecycle of cubes, such as creating/updating/deleting network services.

In addition, it provides a single point of entry (a rest API server) for the configuration of any network function.

The preferred way to interact with polycubed is through :doc:`polycubectl <../polycubectl/polycubectl>`.

The Polycube System Daemon ``polycubed`` provides a kernel abstraction layer that is used by the different services.
It exposes a configuration mechanism of the different service instances through a rest API server.  Users can interact with it using :doc:`polycubectl <../polycubectl/polycubectl>`.

It requires root privileges and only an instance can be launched system wide.


Systemd integration
^^^^^^^^^^^^^^^^^^^

``polycubed`` can be managed as a systemd service.

::

    # start the service
    sudo systemctl start polycubed

    # stop the service
    sudo systemctl stop polycubed

    # restart the service
    sudo systemctl reload-or-restart polycubed

    # enable the service to be started at boot time
    sudo systemctl enable polycubed

    # see service status
    sudo systemctl status polycubed

    # check logs
    journalctl -u polycubed // '-f' can be used to see a live version

Usage
^^^^^

::

    Usage: polycubed [OPTIONS]
    -p, --port PORT: port where the rest server listens (default: 9000)
    -a, --addr: addr where polycubed listens (default: localhost)
    -l, --loglevel: set log level (trace, debug, info, warn, err, critical, off, default: info)
    -c, --cert: path to ssl certificate
    -k, --key: path to ssl private key
    --cacert: path to certification authority certificate used to validate clients
    -d, --daemon: run as daemon (default: false)
    -v, --version: show version and exit
    --logfile: file to save polycube logs (default: /var/log/polycube/polycubed.log)
    --pidfile: file to save polycubed pid (default: /var/run/polycube.pid)
    --configfile: configuration file (default: /etc/polycube/polycube.conf)
    --cert-black-list: path to black listed certificates
    --cert-white-list: path to white listed certificates
    -h, --help: print this message


Configuration file
^^^^^^^^^^^^^^^^^^

The configuration file is and alternative way to configure polycubed.
It is a text file using a ``parameter: value`` syntax, where parameter refers to the long ones described above.
By default it resides in ``/etc/polycube/polycubed.conf``, this location can be changed by using the ``--configfile`` parameter.
A default file will be created if it does not exist and the ``--configfile`` parameter is not specified.
If the same parameter is specified in both, the configuration file and the command line, the later one is considered.
``polycubed`` has to be restarted after the configuration file is updated in order to take the new values.

::

    # polycubed configuration file
    # this is a comment
    loglevel: info
    daemon: true
    #p: 6000 <-- this is NOT supported, only long options are



Persistency
^^^^^^^^^^^

Polycubed has persistent capabilities which means that (1) it can automatically load the configuration that was present when the daemon was shut down, (2) each time a configuration command is issued, it is automatically dumped on disk.
This allows polycubed also to recover from failures, such as rebooting the machine.
To enable this feature we need to start polycubed with the ``--cubes-dump-enable`` flag.
The daemon keeps in memory an instance of all the topology, including the configuration of each individual service.
Topology and configuration are automatically updated at each new command; the configuration is also dumped to disk, on file ``/etc/polycube/cubes.yaml``.
The standard behavior of the daemon at startup is to load, if present, the latest topology that was active at the end of the previous execution.
Users can load a different topology file by using the ``--cubes-dump-file`` flag followed by the path to the file.
In case we want to start polycubed with an empty topology, avoiding any possible load at startup, we can launch polycubed with the ``--cubes-dump-clean-init`` flag. Beware that in this case any existing configuration in the default file will be overwritten.
``--cubes-dump-enable`` is required if we want to use any of the other two related flags.
There are some limitations: (1) YANG actions, such as "append" for firewall and nat rules, are not supported, (2) some services fail to load the full configuration at once and (3) transparent services attached to netdevs are not saved in the cubes dump file.

::

    # start daemon with topology saving functionalities
    polycubed --cubes-dump-enable

    # start polycubed with custom cubes configuration
    polycubed --cubes-dump-enable --cubes-dump-file ~/Desktop/myCubes.yaml

    # start polycubed with an empty topology
    polycubed --cubes-dump-enable --cubes-dump-clean-init


.. _polycubed-debugging:

Debugging
^^^^^^^^^
The debugging of polycubed can be turned on by starting the daemon with the ``--loglevel=debug`` flag.


Rest API
^^^^^^^^

Here is the detailed documentation of the :doc:`REST API <../developers/rest-api>`. 
