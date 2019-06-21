polycubed: the Polycube System Daemon
=====================================

.. The Polycube system daemon (polycubed) is in charge of managing the lifecycle of cubes, such as creating/updating/deleting network services.

.. In addition, it provides a single point of entry (a rest API server) for the configuration of any network function.

.. The preferred way to interact with polycubed is through `polycubectl <../polycubectl.rst>`_.

The Polycube System Daemon ``polycubed`` provides a kernel abstraction layer that is used by the different services.
It exposes a configuration mechanism of the different service instances through a rest API server.  Users can interact with it using the :ref:`polycubectl CLI <polycubectl>`

It requires root privileges and only an instance can be launched system wide.


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

Polycubed has persistent capabilities, which means that (1) it can automatically load the configuration that was present when the daemon was shut down, (2) each time a configuration command is issued, it is automatically dumped on disk.
This enables polycubed also to recover from failures, such as rebooting the machine.
By default, the daemon keeps in memory an instance of all the topology, including the configuration of each individual service.
Topology and configuration are automatically updated at each new command; the configuration is also dumped to disk, on file ``/etc/polycube/cubes.yaml``.
The standard behavior of the daemon at startup is to load the latest topology that was active at the end of the previous execution.
Users can load a different topology file by using the ``--cubes-dump`` flag followed by the path to the file.
In case we want to start polycubed with an empty topology, avoiding any possible load at startup, we can launch polycubed with the ``--cubes-init`` flag. Beware that in this case any existing configuration in the default file will be overwritten.
Finally, persistency can be disabled with the ``--cubes-nodump`` flag; this would also avoid any (very limited) performance penalty introduced by this feature.

::

    # start polycubed with custom cubes configuration
    polycubed --cubes-dump ~/Desktop/myCubes.yaml

    # start polycubed with an empty topology
    polycubed --cubes-init

    # start daemon without topology saving functionalities
    polycubed --cubes-nodump



Rest API
^^^^^^^^

TODO...
