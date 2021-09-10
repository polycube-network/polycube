# Polyube metrics


**In brief**

We have defined a way to be able to add metrics to each service dynamically, without heavily modifying the parser.

## Done

- [X] The core of prometheus-cpp library has been added 
- [X] Extensions added in polycube-base.yang to support metrics in OpenMetrics format
- [X] Data structures have been added in ServiceMetadata in management_interface.h file
- [X] Metrics (Counter, Gauge) added in the yang of some services
- [X] Prototypes in Yang.h then in YangNodes.tcpp have been changed (ServiceMetadata* md parameter has been added) to allow the saving of the metrics read by the yang that will eventually be used on the rest server
- [X] Adding code in some parser methods to read metric extensions and to save
- [X] In the rest server (header) the data structures that are used to support, collect and record metrics have been defined.
- [X] A function has been created in the rest server that fills the data structures once to record the metrics read by the service yangs
- [X] Thanks to get_metrics all read metrics are exported to an endpoint /metrics,
- [X] The jsoncons library has been added (thanks to jsonpath written in the extensions of a yang file you can access the metric value through the json of a service)
- [X] Access the entire json of a service and use jsonpath to read the value of a metric
- [X] Definition of metrics per cube thanks to the use of labels (example: cubeName=d1)
- [] Add metrics to other services
- [X] Add tests



## Prometheus-cpp

The library chosen to create metrics was [Prometheus-cpp](https://github.com/jupp0r/prometheus-cpp.git) . In brief, from the official documentation: this library aims to enable Metrics-Driven Development for C++ services. It implements the Prometheus Data Model, a powerful abstraction on which to collect and expose metrics. We offer the possibility for metrics to be collected by Prometheus, but other push/pull collections can be added as plugins.
The whole library was not used, in fact only the core part is necessary for the creation and maintenance of the metrics since the exposure of the metrics is done using the rest server with Pistache. 


Extensions added for metrics in OpenMetrics format:

- polycube-base:name-metric
- polycube-base:type-metric
- polycube-base:help-metric


In addition:

- polycube-base:path-metric: this extension has been added to get the value of a metric from an instantiated service
- polycube-base:type-operation: this extension has been added to overcome the problem that jsonpath does not allow (in a single query) to apply the "length" operator after a filter and in some cases we need it. Maybe it's not the best solution so maybe in the future it will be improved in some other way.


## Jsoncons

The library chosen to obtain the value of a metric from the json of an instanced cube used the jsonpath is [jsoncons] (https://github.com/danielaparker/jsoncons).
In brief, from the official documentation: jsoncons is a C++, header-only library for constructing JSON and JSON-like data formats such as CBOR. 

In particular we use the extension of jsonpath that implements Stefan Goessner's JSONPath. It also supports search and replace using JSONPath expressions.



## How to add metrics to a service

To add a metric to a service you must add the extensions defined in polycube-base.yang to the chosen point of the service yang file (which can be a leaf, a container or a list):

- polycube-base:name-metric: you must respect OpenMetrics rules
- polycube-base:type-metric: GAUGE, COUNTER
- polycube-base:help-metric: a summary to help those who read the metrics

- polycube-base:path-metric: from the polycube-codegen output file (with a few modifications) you have to take the jsonpath up to the point where the metric is written. Another way is to use polycubectl to create a cube with the desired characteristics, print the json of the cube and derive the jsonpath. It is possible to use this [tool](https://jsonpath.com/) online

- polycube-base:typo-operation: in some cases you jsonpath is not enough to get the correct value for a metric (does not allow to apply "length" on a filtered json)


## Future

As is natural, there are improvements to be made and also new proposals presented on the current implementation:

- remove the type-operation annotation
- add a file (maybe even a yang) linked to the yang of a service, where you can write a list of possible metrics using the extensions already defined. This would bring enormous advantages since with the current implementation it is possible to define only one metric for each element of the yang that is a leaf or list or similar.
- introduce the ability to add metrics directly using polycubectl

