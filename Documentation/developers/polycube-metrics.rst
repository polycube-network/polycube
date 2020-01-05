Polyube metrics with Prometheus
=================================


Prometheus-cpp
---------------
The library chosen to create metrics was `Prometheus-cpp <https://github.com/jupp0r/prometheus-cpp.git>`_ which was added as a submodule to Polycube, modifying the gitsubmodules file and the CMakeLists.txt in polycube/src/polycubed/src (a comment for the future has also been added in the file scripts/pre-requirements.sh)

Extensions added for metrics in OpenMetrics format:
- polycube-base:name-metric
- polycube-base:type-metric
- polycube-base:help-metric

In addition, this extension was added to get the value of a metric from an instantiated service:
- polycube-base:path-metric


Jsoncons
--------
The library chosen to obtain the value of a metric from the json of an instanced service used the jsonpath is `jsoncons <https://github.com/danielaparker/jsoncons>`_

How to add metrics to a service
--------------------------------
To add a metric to a service you must add the extensions defined in polycube-base.yang to the chosen point of the service yang file (which can be a leaf, a container, a list, an augment or similar):

- polycube-base:name-metric: you must respect OpenMetrics rules

- polycube-base:type-metric: gauge, counter, histogram, summary

- polycube-base:help-metric: a summary to help those who read the metrics

- polycube-base:path-metric: from the polycube-codegen output file (with a few modifications) you have to take the jsonpath up to the point where the metric is written.





Done
----------
- [X] The prometheus-cpp library has been added (for now as a submodule) https://github.com/jupp0r/prometheus-cpp
- [X] Extensions added in polycube-base.yang to support metrics in OpenMetrics format
- [X] Data structures have been added in ServiceMetadata in management_interface.h file
- [X] Metrics (Counter, Gauge) added in the yang of some services (for now only in leaf and list)
- [X] Prototypes in Yang.h then in YangNodes.tcpp have been changed (ServiceMetadata * md parameter has been added) to allow saving metrics read in yang as metadata defined in management_interface and which can be accessed using a function defined in service_controller which can in turn be called by a core element in the rest server
- [X] Adding code in ParseLeaf and ParseList to read metric extensions and save
- [X] In the rest server (header) the data structures that are used to support, collect and record metrics have been defined.
- [X] A function has been created in the rest server that fills the data structures once to record the metrics read by the service yangs
- [X] Thanks to get_metrics all read metrics are exported to an endpoint / metrics, for now with null value
- [X] The jsoncons library has been added (for now as a submodule) (thanks to jsonpath written in the extensions of a yang file you can access the metric value through the json of a service) https://github.com/ danielaparker / jsoncons
- [X] Document the addition of metrics to an existing service


TODO
-----------
- [] Add extensions in augment, containers and other parts (some extensions that are currently in the leaves will be put in augment)
- [] Create a ParseAugment (maybe there are problems with the used version of libyang)
- [] Access the entire json of a service and use jsonpath to read the value of a metric (with get_cube I can access only the base cube of a service)
- [?] Move the prometheus-cpp and jsonpath libraries to External Libraries
- [] Comment on code in YangNodes.tcpp
- [] Obtain from the output files of polycube-codegen all the jsonpaths for the metrics and add them (update them) to the extensions in the yang
- [] Use the metric types Histogram and Summary
- [] In the rest_server access all cubes given the name of a service
- [] Continue the documentation
