Quick Start
===========

If you want to try Polycube, this is a five minutes document that will guide you through the basics.

In order to try Polycube you need to use two components:

- ``polycubed``: the :doc:`Polycube daemon <polycubed/polycubed>`, that must be up and running.
- ``polycubectl``: the :doc:`Polycube CLI <polycubectl/polycubectl>`, used to interact with the framework.

There are two modes to try Polycube: using :ref:`sec-docker` or installing it on the :ref:`sec-bare-metal`.

.. _sec-docker:

Docker
------

Docker is the easiest and fastest way to try Polycube because it only requires a recent kernel (see  :ref:`sec-updating-linux-kernel` for more information).

You can use Polycube by pulling the proper Docker image, either the **most recent released version (stable)** or the **most recent snapshot (from GitHub)** (note that the above commands may require root privileges, i.e., ``sudo``):

- **Released version** (stable):

  ::

      docker pull polycubenetwork/polycube:v0.9.0-rc

- **Most recent snapshot** (from GitHub)

  ::

     docker pull polycubenetwork/polycube:latest


Run the Polycube Docker and launch ``polycubed`` (the polycube daemon) inside it, as follows:

- **Released version** (stable):

  ::

      docker run  -it --rm --privileged --network host \
      -v /lib/modules:/lib/modules:ro -v /usr/src:/usr/src:ro -v /etc/localtime:/etc/localtime:ro \
      polycubenetwork/polycube:v0.9.0-rc /bin/bash -c 'polycubed -d && /bin/bash'


- **Most recent snapshot** (from GitHub)
  ::

      docker run  -it --rm --privileged --network host \
      -v /lib/modules:/lib/modules:ro -v /usr/src:/usr/src:ro -v /etc/localtime:/etc/localtime:ro \
      polycubenetwork/polycube:latest /bin/bash -c 'polycubed -d && /bin/bash'

The Docker container is launched in the host networking stack (``--network host``) and in privileged mode (``--privileged``), which are required to use ``eBPF`` features.
The above command starts ``polycubed`` (with standard flags) and opens a shell, which can be used to launch ``polycubectl``. Refer to :doc:`polycubectl CLI <polycubectl/polycubectl>` for more information about the CLI interface.

::

    polycubectl --help

To stop the daemon just remove the container, e.g. exiting from the shell with the ``exit`` command.


.. _sec-bare-metal:

Bare Metal
----------

This is a more elaborated way, recommended either for *advanced users* or  for who would like to use Polycube in production (*not for testing*).
Please refer to the :doc:`installation guide <installation>` to get detailed information about how to compile and install Polycube and its dependencies.

Once you have Polycube installed in your system, you can start the ``polycubed`` service:

::

    # start polycubed service
    # (sudo service start polycubed will work in many distros as well)
    sudo systemctl start polycubed

    # check service status
    sudo systemctl status polycubed

Start interacting with the framework by using ``polycubectl``. Refer to :doc:`polycubectl CLI <polycubectl/polycubectl>` for more information.

::

    polycubectl --help


To stop the ``polycubed`` service use:

::

    sudo systemctl stop polycubed
