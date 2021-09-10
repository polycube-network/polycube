# Dynamic network monitor 


Dynmon (``dynmon``) is a transparent service that allows the dynamic injection of eBPF code in the linux kernel, enabling the monitoring of the network traffic and the collection and exportation of custom metrics.

This service exploits the capabilities of Polycube to replace the eBPF code running in the dataplane and the use of eBPF maps to share data between the control plane and the data plane.

## Features

- Transparent service, can be attached to any network interface and Polycube services
- Support for the injection of any eBPF code at runtime
- Support for eBPF maps content exportation through the REST interface as metrics
- Support for two different exportation formats: JSON and OpenMetrics
- Support for both INGRESS and EGRESS program injection with custom rules
- Support for shared maps between INGRESS and EGRESS
- Support for atomic eBPF maps content read thanks to an advanced map swap technique
- Support for eBPF maps content deletion when read

## Limitations

- The OpenMetrics format does not support complex data structures, hence the maps are exported only if their value type is a simple type (structs and unions are not supported)
- The OpenMetrics Histogram and Summary metrics are not yet supported
- Data extraction is possible only in the following maps (as listed in [MapExtractor.cpp#L287](https://github.com/polycube-network/polycube/blob/master/src/services/pcn-dynmon/src/extractor/MapExtractor.cpp#L287)):
  - BPF_MAP_TYPE_HASH, BPF_MAP_TYPE_PERCPU_HASH
  - BPF_MAP_TYPE_LRU_HASH, BPF_MAP_TYPE_LRU_PERCPU_HASH,
  - BPF_MAP_TYPE_ARRAY, BPF_MAP_TYPE_PERCPU_ARRAY
  - BPF_MAP_TYPE_QUEUE, BPF_MAP_TYPE_STACK

Furthermore, optimized data extraction (the so called *batch operations*), are supported only by BPF_MAP_TYPE_HASH, BPF_MAP_TYPE_LRU_HASH, BPF_MAP_TYPE_ARRAY.


## How to use


### Creating the service

```
#create the dynmon service instance
polycubectl dynmon add monitor
```

### Configuring the data plane

In order to configure the dataplane of the service, a configuration JSON object must be sent to the control plane; this action cannot be done through the **polycubectl** tool as it does not handle complex inputs.

To send the data plane configuration to the control plane it is necessary to exploit the REST interface of the service, applying a ``PUT`` request to the ``/dataplane-config`` endpoint.

Configuration examples can be found in the *examples* directory.


### Attaching to a interface

```
# Attach the service to a network interface
polycubectl attach monitor eno0

# Attach the service to a cube port
polycubectl attach monitor br1:toveth1
```

### Collecting metrics

To collect the metrics of the service, two endpoints have been defined to enable the two possible exportation formats:

- JSON format

    ```
    polycubectl monitor metrics show
    ```

- OpenMetrics format

    ```
    polycubectl monitor open-metrics show
    ```

This way, the service will collect all define metrics both for Ingress and Egress program, but if you just want to narrow it down to only one type you can type a more specific command like:

```
polycubectl monitor metrics ingress-metrics show
```

For any other endpoint, try the command line suggestions by typing ``?`` after the command you want to dig more about.


## Advanced configuration


For every metric you decide to export you can specify some additional parameter to perform advanced operations:

- Swap the map when read

- Empty the map when read


The JSON configuration will like like so (let's focus only on the Ingress path):

```
{
    "ingress-path": {
        "name": "...",
        "code": "...",
        "metric-configs": [
            {
                "name": "...metric_name...",
                "map-name": "...map_name...",
                "extraction-options": {
                     "swap-on-read": true,
                    "empty-on-read": true
                }
            }
        ]
    },
    "egress-path": {}
}
```

The parameter ``empty-on-read`` simply teaches Dynmon to erase map content when read. Depending on the map type, the content can be whether completely deleted (like in an HASH map) or zero-ed (like in an ARRAY). This is up to the user, to fit its needs in some more complex scenario than a simple counter extraction.

Concerning the ``swap-on-read`` parameter, please check the :ref:`sec-code-rewriter` section, where everything is detailed. To briefly sum it up, this parameter allows users to declare swappable maps, meaning that their read is performed ATOMICALLY with respect to the DataPlane (which normally would continue to insert/modify values in the map) thanks to these two steps:

- when the code is injected, the CodeRewriter checks for any maps declared with this parameter and optimizes the code, creating dummy parallel maps to be used later on;

- when the user requires a swappable map content, Dynmon alternatively modifies the current map pointer to point to the original/fake one.


This way, the user will still be able to require the metric he declared as he would normally do, and Dynmon will perform that read atomically swapping the maps under the hoods, teaching DataPlane to use the other parallel one.


## Dynmon Injector Tool


This tool allows the creation and the manipulation of a `dynmon` cube without using the standard `polycubectl` CLI.

### Install

Some dependencies are required for this tool to run:
```
pip install -r requirements.txt
```

### Running the tool

```
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
```

### Usage examples

```
basic usage:
./dynmon_injector.py monitor_0 eno1 ../examples/packet_counter.json

setting custom ip address and port to contact the polycube daemon:
./dynmon_injector.py -a 10.0.0.1 -p 5840 monitor_0 eno1 ../examples/packet_counter.json
```

This tool creates a new `dynmon` cube with the given configuration and attaches it to the selected interface.

If the monitor already exists, the tool checks if the attached interface is the same used previously; if not, it detaches the cube from the previous interface and attaches it to the new one; then, the selected dataplane is injected.


## Dynmon Extractor tool


This tool allows metric extraction from a `dynmon` cube without using the standard `polycubectl` CLI.

### Install

Some dependencies are required for this tool to run:

```
pip install -r requirements.txt
```

### Running the tool

```
usage: dynmon_extractor.py [-h] [-a ADDRESS] [-p PORT] [-s] [-t {ingress,egress,all}] [-n NAME]
                       cube_name

positional arguments:
  cube_name             indicates the name of the cube

optional arguments:
  -h, --help            show this help message and exit
  -a ADDRESS, --address ADDRESS
                        set the polycube daemon ip address (default: localhost)
  -p PORT, --port PORT  set the polycube daemon port (default: 9000)
  -s, --save            store the retrieved metrics in a file (default: False)
  -t {ingress,egress,all}, --type {ingress,egress,all}
                        specify the program type ingress/egress to retrieve the metrics (default: all)
  -n NAME, --name NAME  set the name of the metric to be retrieved (default: None)
```

### Usage examples

```
basic usage:
./dynmon_extractor.py monitor_0

only ingress metrics and save to json:
./dynmon_extractor.py monitor_0 -t ingress -s
```

## Code Rewriter


The Code Rewriter is an extremely advanced code optimizator to adapt user dynamically injected code according to the provided configuration. It basically performs some optimization in order to provide all the requested functionalities keeping high performance and reliability. Moreover, it relies on eBPF code patterns that identify a map and its declaration, so the user does not need to code any additional informations other than the configurations for each metric he wants to retrieve.

First of all, the Rewriter could be accessible to anyone, meaning that other services could use it to compile dynamically injected code, but since Dynmon is the only Polycube's entry point for user code by now, you will se its usage limited to the Dynmon service. For future similar services, remember that this rewriter is available.

The code compilation is performed every time new code is injected, both for Ingress and Egress data path, but actually it will optimize the code only when there is at least one map declared as ``"swap-on-read"``. Thus, do not expect different behaviour when inserting input without that option.

There are two different type of compilation:

- PROGRAM_INDEX_SWAP

- PROGRAM_RELOAD
  
### PROGRAM_INDEX_SWAP rewrite


The PROGRAM_INDEX_SWAP rewrite type is the best you can get from this rewriter by now. It is extremely sophisticated and not easy at all to understand, since we have tried to take into account as many scenarios as possible. This said, let's analyze it.

During the first phase of this compilation, all the maps declared with the ``"swap-on-read"`` feature enabled are parsed, checking if their declaration in the code matches one of the following rules:

- the map is declared as _SHARED
- the map is declared as _PUBLIC
- the map is declared as _PINNED
- the map is declared as "extern"

Since those maps are declared as swappable, if any of these rules is matched, then the rewriter declares another dummy map named ``MAP_NAME_1`` of the same time, which will be used when the code is swapped. Although, in case the map was _PINNED, the user have to be sure that another pinned map named ``MAP_NAME_1`` is present and created a priori in the filesystem, since this rewriter cannot create a _PINNED map for you. For all these other types, another parallel map is created smoothly.

If a user created a map of such type, then he probably wants to use another previously declared map out or inside Polycube, or he wanted to share this map between Ingress and Egress programs.

If the map did not match one of these rules, then it is left unchanged in the cloned code, meaning that there will be another program-local map with limited scope that will be read alternatively.

The second phase consists is checking all those maps which are not declared as swappable. The rewriter retrieve all those declarations and checks for them to see if it is able to modify it. In fact, during this phase, whenever it encounters a declaration which it is unable to modify, it stops and uses the PROGRAM_RELOAD compilation as fallback, to let everything run as required, even though in an sub-optimal optimized way.

Since those map must not swap, the rewriter tries to declare a map which is shared among the original and cloned program, in order to make the map visible from both of them. For all those maps, these rules are applied:

- if the map is declared as _PINNED or "extern", then it will be left unchanged in the cloned program, since the user is using an extern map which should exists a priori
- if the map is NOT declared using the standard (BPF_TABLE and BPF_QUEUESTACK) helpers, then the compilation stops and the PROGRAM_RELOAD one is used, since the rewriter is not able by now to change such declarations into specific one (eg. from BPF_ARRAY(...) to BPF_TABLE("array"...), too many possibilities and variadic parameters)
- if the map is declared as _SHARED or _PUBLIC, then the declaration is changed in the cloned code into "extern", meaning that the map is already declared in the original code
- otherwise, the declaration in the original code is changed into BPF_TABLE_SHARED/BPF_QUEUESTACK_SHARED and in the cloned code the map will be declared as "extern". Moreover, the map name will be changed into ``MAP_NAME_INGRESS`` or ``MAP_NAME_EGRESS`` to avoid such much to collide with others declared in a different program type.

Once finished, both the original and cloned code are ready to be inserted in the probe. Since it is an enhanced compilation which allows users to save time every time they want to read their metrics, we have used a very efficient technique to alternate the code execution. These two programs are compiled also from LLVM one time, and then they are inserted in the probe but not as primarly code. In fact, this compilation delivers also a master PIVOTING code which will be injected as code to be executed every time there is an incoming/outgoing packet.

The PIVOTING code simply calls the original/cloned program main function according to the current program index. This program index is stored in an internal BPF_TABLE and it is changed every time a user performs a read. When the index refers to the original code, the PIVOTING function will call the original code main function, and vice versa.

Thanks to this technique, every time a user requires metrics there's only almost 4ms overhead due to changing the index from ControlPlane, which compared to the 400ms using the PROGRAM_RELOAD compilation, is an extremely advantage we are proud of having developed.


### PROGRAM_RELOAD compilation


This compilation type is quite simple to understand. It is used as a fallback compilation, since it achieves the map swap function, but in a more time expensive way. In fact, when this option is used, it is generated a new code starting from the original injected one, and then the following steps are followed:

1. in the cloned code, change all ``MAP_NAME`` occurrences with opportunistic names to distinguish them, like ``MAP_NAME_1``
2. in the cloned code, add the original ``MAP_NAME`` declaration that is present in the original code
3. in the original code, add the ``MAP_NAME_1`` declaration that is present in the cloned code

Since we have to guarantee map read atomicity, we declare a new parallel map with a dummy name. Whenever the user requires metrics, the currently active code is swapped with inactive one, meaning that all the map usages are "replaced" with the dummy/original ones (eg. MAP.lookup() will become MAP_1.lookup() alternatively). Whenever the code is swapped, all the other maps which were not declared as swappable are kept, thanks to the advanced Polycube's map-stealing feature. This way their content is preserved, and the only thing that changes is, as required by the user, the swappable maps' ones.

Both the new and old map declaration need to be places in the codes, otherwise they would not know about the other maps other than the ones they have declared.

The codes are, as said, alternatively injected in the probe, but it is worth noticing that although the PROGRAM_INDEX_SWAP compilation, this one requires LLVM to compile the current code every time it is swapped.

Some tests have been run and their results led to 400ms on average of overhead each time the user requires metrics, due to the LLVM compilation time and the time to inject the code in the probe. Obviously, it is not the better solution, but at least it provides the user all the functionality he asked for, even though the enhanced compilation went wrong.
