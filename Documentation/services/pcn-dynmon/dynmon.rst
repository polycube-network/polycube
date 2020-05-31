.. _rst_dynmon:

Dynamic network monitor (``dynmon``)
====================================

Dynmon is a transparent service that allows the dynamic injection of eBPF code in the linux kernel, enabling the monitoring of the network traffic and the collection and exportation of custom metrics.

This service exploits the capabilities of Polycube to replace the eBPF code running in the dataplane and the use of eBPF maps to share data between the control plane and the data plane.

Features
--------
- Transparent service, can be attached to any network interface and Polycube services
- Support for the injection of any eBPF code at runtime
- Support for eBPF maps content exportation through the REST interface as metrics
- Support for two different exportation formats: JSON and OpenMetrics

Limitations
-----------
- The OpenMetrics format does not support complex data structures, hence the maps are exported only if their value type is a simple type (structs and unions are not supported)
- The OpenMetrics Histogram and Summary metrics are not yet supported

How to use
----------


Creating the service
^^^^^^^^^^^^^^^^^^^^
::

    #create the dynmon service instance
    polycubectl dynmon add monitor


Configuring the data plane
^^^^^^^^^^^^^^^^^^^^^^^^^^
In order to configure the dataplane of the service, a configuration JSON object must be sent to the control plane; this action cannot be done through the **polycubectl** tool as it does not handle complex inputs.

To send the data plane configuration to the control plane it is necessary to exploit the REST interface of the service, applying a ``PUT`` request to the ``/dataplane`` endpoint.

Configuration examples can be found in the *examples* directory.


Attaching to a interface
^^^^^^^^^^^^^^^^^^^^^^^^^^^
::

    # Attach the service to a network interface
    polycubectl attach monitor eno0

    # Attach the service to a cube port
    polycubectl attach monitor br1:toveth1


Collecting metrics
^^^^^^^^^^^^^^^^^^
To collect the metrics of the service, two endpoints have been defined to enable the two possible exportation formats:

- JSON format

    ::

        polycubectl monitor metrics show

- OpenMetrics format

    ::

        polycubectl monitor open-metrics show


Dynmon Injector Tool
--------------------

This tool allows the creation and the manipulation of a `dynmon` cube without using the standard `polycubectl` CLI.

Install
^^^^^^^
Some dependencies are required for this tool to run:
::

    pip install -r requirements.txt


Running the tool
^^^^^^^^^^^^^^^^
::

    Usage: `dynmon_injector.py [-h] [-a ADDRESS] [-p PORT] [-v] cube_name peer_interface path_to_dataplane`
    
    positional arguments:
    cube_name             indicates the name of the cube
    peer_interface        indicates the network interface to connect the cube to
    path_to_dataplane     indicates the path to the json file which contains the new dataplane configuration
                          which contains the new dataplane code and the metadata associated to the exported metrics

    optional arguments:
    -h, --help                        show this help message and exit
    -a ADDRESS, --address ADDRESS     set the polycube daemon ip address (default: localhost)
    -p PORT, --port PORT              set the polycube daemon port (default: 9000)
    -v, --version                     show program's version number and exit


Usage examples
^^^^^^^^^^^^^^
::

    basic usage:
    ./dynmon_injector.py monitor_0 eno1 ../examples/packet_counter.json

    setting custom ip address and port to contact the polycube daemon:
    ./dynmon_injector.py -a 10.0.0.1 -p 5840 monitor_0 eno1 ../examples/packet_counter.json


This tool creates a new `dynmon` cube with the given configuration and attaches it to the selected interface.

If the monitor already exists, the tool checks if the attached interface is the same used previously; if not, it detaches the cube from the previous interface and attaches it to the new one; then, the selected dataplane is injected.