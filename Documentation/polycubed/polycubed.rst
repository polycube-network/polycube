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

Rest API
^^^^^^^^

TODO...