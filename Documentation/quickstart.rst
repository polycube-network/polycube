Quick Start
===========

If you want to try Polycube, this is a 5 minutes document that will guide you through the basics.

In order to try polycube you need to use two components:

- ``polycubed``: the :doc:`Polycube daemon <polycubed/polycubed>`, that must be up and running.
- ``polycubectl``: the :doc:`Polycube CLI <polycubectl/polycubectl>`, used to interact with the framework.

There are two modes to try ``polycube``: Using Docker or installing it on the bare metal.

Docker
------

Docker is the easiest and fastest way to try ``polycube`` if you are a new user because it only requires to have a recent kernel version. See :ref:`updating-linux-kernel`.

Pull Polycube Docker image

::

    docker pull polycubenetwork/polycube:latest


Run the Polycube Docker and launch ``polycubed`` (the polycube daemon) inside it.
The Docker container is launched in the host networking stack (``--network host``), privileged mode (``--privileged``) is necessary to use ``eBPF`` features.

::

    docker run  -it --rm --privileged --network host \
    -v /lib/modules:/lib/modules:ro -v /usr/src:/usr/src:ro -v /etc/localtime:/etc/localtime:ro \
    polycubenetwork/polycube:latest /bin/bash -c 'polycubed -d && /bin/bash'


``polycubectl`` is available inside the container. Refer to :doc:`polycubectl CLI <polycubectl/polycubectl>`.

::

    polycubectl --help

To stop the daemon just remove the container. For example closing its terminal with the ``exit`` command.

Bare Metal
----------

This is a more elaborated way only recommended for advance users.
Please the :doc:`installation guide <installation>` to get detailed information about how to compile and install ``polycube`` and its dependencies.

Once you have ``polcyube`` installed in your system, you can start the ``polycubed`` service:

::

    # start polycubed service
    # (sudo service start polycubed will work in many distros as well)
    sudo systemctl start polycubed

    # check service status
    sudo systemctl status polycubed

Start interacting with the framework by using ``polycubectl``. Refer to :doc:`polycubectl CLI <polycubectl/polycubectl>`.

::

    polycubectl --help


To stop the ``polycubed`` service use:

::

    sudo systemctl stop polycubed
