Polycube Developers Guide
=========================

Note: This document is outdated. md to rst conversion is not totally done and the contents could be not correct.

This guide represents an initial starting point for developers that want to implement new services (e.g., custom NAT, router, etc) using the Polycube software infrastructure.

Common code structure for Polycube services
-------------------------------------------

All the services provided with Polycube have a fixed structure for their source code.
Although this common structure is not mandatory, we exploit an automatic code generation process to create the skeleton of new services to facilitate the implementation of the control plane.

Due to the difficulties in creating a complete control plane, which need to be compatible with what the `polycubed` expects (bear in mind that the Polycube daemon is service-independent), we recommend (1) to use the automatic code generation we describe in [devel/README.md](devel/README.md), and (2) keep the same structure of the source code as shown here.

Code Structure
^^^^^^^^^^^^^^
- **src/api/{service-name}Api.[h,cpp]**: This class implements the ``ManagementInterface`` that is used by `polycubed` to send management commands to the different services. It contains a REST server router that forwards request to the correct handler, and it performs the conversion between HTTP objects and their JSON object representation, and vice-versa. Finally, this class provides the help API used by the CLI to print informative messages when the `?` keyword is invoked. The real implementation of the handlers is present in the next class.
- **src/api/{service-name}ApiImpl.[h,cpp]**: This class contains all the instances of the service. It handles the creation and destruction of new instances as also the creation and destruction of ports. For the other API methods, it calls the correct method to satisfy the request on the correct instance.
- **src/interface/**: This folder contains one pure virtual class for each object used in the control API. Each class contains the methods that must be implemented in each object class.
- **src/serializer/**: This folder contains one JSON object class for each object used in the control API. These classes are used to performs the marshalling/unmarshalling operations.
- **src/{object-name}.h**: One header file for each object used in the control API. These classes implement the corresponding object interface stored in the `src/interface` folder. Each header file is included in two different `.cpp`, `{object-name}DefaultImpl.cpp` and `{object-name}.cpp`.
- **src/default-src/{object-name}DefaultImpl.cpp**: These files contain the implementation of methods to handle sub-objects. They simply call the sub-object method to create, delete, replace and read it.
- **src/{object-name}.cpp**: These files contain the getter and setter methods, constructor and destructor and finally some methods to handle the object life cycle such as create, delete, replace, update and read. Some methods contain a basic implementation, others instead have to be implemented.
- **src/{service-name}_dp.h**: Contains the datapath code for the service.
- **src/{service-name]-lib.cpp**: Is used to compile the service as shared library.
- **.swagger-codegen-ignore**: This file is used to prevent files from being overwritten by the generator if a re-generation is performed.

How to create a new service / update an existing one
----------------------------------------------------

The process to create a new service could be summarized in these steps:
  - (1) Write a datamodel for the service
  - (2) Use the datamodel for generating a service stub (or)
  - (2b) Updating an existing service stub
  - (3) Implement the eBPF datapath
  - (4) Implement the control plane

Please note that steps (1) and (2) are needed only when we write a completely new service, while (2b) applies when we need to update the YANG data model of an existing service, hence partially regenerate the code skeleton.
In case we have to slightly modify an existing service (e.g., fixing a bug in the code) that does not involve the YANG data model, usually only steps (3) and (4) are required.


1. Writing a datamodel for the service
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The service structure is described using a YANG data model.
All the services intended to work with polycube must derive from a based service definition that defines some common structures across all services (e.g., _ports_).

In order to derive from such base datamodel include the following line:

::

  import polycube-base { prefix "basemodel"; }


In addition, if the base definition of a port is not enough for the service, it is possible to use the `augment` keyword to insert additional nodes, as shown below:

::

  uses "basemodel:base-yang-module" {
    augment ports {
    // Put here additional port's fields.
    }
  }


The easiest way for starting to write a datamodel is to take a look to the existing ones: [helloworld](services/pcn-helloworld/resources/helloworld.yang), [forwarder](services/pcn-forwarder/resources/forwarder.yang) and [bridge](services/pcn-bridge/resources/bridge.yang).

Documentation about YANG can be found on `RFC6020 <https://tools.ietf.org/html/rfc6020>`_ and `RFC7950 <https://tools.ietf.org/html/rfc7950>`_.


2. Generating (or updating) a service stub
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Please follow the instructions available in :ref:`codegen` in order to understand how to generate code of your `Polycube` service using the automatic code generation.

Our tools for automatic code generation will automatically generate all the REST API of your service starting from the YANG, leaving only a few details to customize.
This will save a lot of time since developers do not have to create the code that validates input parameters, or implement all the HTTP methods that provide access (create/read/update/delete) any object defined in the service YANG model.
Instead, developers can concentrated on writing all the service-specific logic (i.e., control plane).


3. Writing the eBPF datapath
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

`eBPF <http://cilium.readthedocs.io/en/latest/bpf/>`_ is an extension of the traditional Berkeley Packet Filter.
The Polycube architecture leverages the software abstraction provided by `BCC <https://github.com/iovisor/bcc/>`_, which is further extended in this project particular with respect to eBPF features that are useful for networking services.
In order to get more information about how to use the maps in BCC please read the `BCC reference guide <https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md>`_, additionally there is a list of the `available eBPF helpers <https://github.com/iovisor/bcc/blob/master/docs/kernel-versions.md>`_.

Polycube architecture adds a wrapper around the user's code, this wrapper calls the `handle_rx` function with the following parameters:

1. **ctx**: Packet to be processed
2. **md**: packet's metadata:

 - **in_port**: integer that identifies the ingress port of the packet.

``polycube`` provides a set of functions to handle the packets, the return value of the `handle_rx` function should be the result of calling one of these functions.

- **pcn_pkt_redirect(struct __sk_buff *skb, struct pkt_metadata *md, u16 port);**: sends the packet through an the ``ifc`` port. [Example](services/pcn-helloworld/src/Helloworld_dp.h#L86)

- **pcn_pkt_drop(struct __sk_buff *skb, struct pkt_metadata *md);**: drops the packet. It is the same that just returning `RX_DROP`. [Example](services/pcn-helloworld/src/Helloworld_dp.h#L78)

- **pcn_pkt_controller(struct __sk_buff *skb, struct pkt_metadata *md, u16 reason)**: sends the packet to the control path controller. Reason can be used to indicate why the packet is being sent to the custom code running in the control path. If there is not any reason `RX_CONTROLLER` could be directly returned. [Example](services/pcn-helloworld/src/Helloworld_dp.h#L82)

- **pcn_pkt_controller_with_metadata(struct __sk_buff *skb, struct pkt_metadata *md, u16 reason, u32 metadata[3])**: Sends the packet to the custom code running in the control path. In addition to the reason the user can also send some additional medatada.

Checksum calculation
********************

The L3 (IP) and L4 (TCP, UDP) checksums has to be updated when fields in the packets are changed.
``polycube`` provides a set of wrappers of the eBPF helpers to do it:

- **`pcn_csum_diff()`**: wrapper of [`BPF_FUNC_csum_diff`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=7d672345ed295b1356a5d9f7111da1d1d7d65867)

    Note: For XDP cubes and kernels version prior to 4.16 this function supports only 4 bytes arguments.

- **`pcn_l3_csum_replace()`**: wrapper of [`BPF_FUNC_l3_csum_replace`](https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/commit/?id=91bc4822c3d61b9bb7ef66d3b77948a4f9177954)

- **`pcn_l4_csum_replace()`**: wrapper of [`BPF_FUNC_l4_csum_replace`](https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/commit/?id=91bc4822c3d61b9bb7ef66d3b77948a4f9177954)

Services as [NAT](services/pcn-nat/src/Nat_dp.h) and [Load Balancer Reverse Proxy](services/pcn-loadbalancer-rp/src/Lbrp_dp.h) shows how to use these functions.


Known limitations:
******************
- It is not possible to send a packet through multiple ports, then multicast, broadcast of any similar functionality has to be implemented in the control path.

TODO:
*****

- Document support for multiple eBPF programs

Debugging and logging in the data plane
***************************************

The polycube framework provides the `pcn_log()` primitive that enables to print messages in the standard log file; hence, all the messages (from both data and control plane) are received in the same place.
This avoids the use of the `bpf_trace_printk()` primitive, which is not integrated with control plane logging.

Syntax:

::

    pcn_log (ctx, level, msg, args...)

- `ctx`: packet being processed.
- `level`: type of logging level: `LOG_TRACE`, `LOG_DEBUG`, `LOG_INFO`, `LOG_WARN`, `LOG_ERR`, `LOG_CRITICAL`.
A brief explanation about when to use each level can be found in `this answer <https://stackoverflow.com/questions/2031163/when-to-use-the-different-log-levels>`_ on StackOverflow
- `msg`: message to print
- `args`: arguments of the message

Two special format specifiers are available:
- %I: print an IP address
- %M: print a MAC address
- %P: print a TCP/UDP port

Please note the the custom specifiers spec the data to be in network byte order while standard specifiers expects it to be in host by order.

Current limitations:
 - Cannot be used inside a macro
 - Maximum 4 arguments are allowed

Usage example:

::

  pcn_log(ctx, LOG_DEBUG, "Receiving packet from port %d", md->in_port);


The ``pcn_pkt_log(ctx, level)`` primitive sends a packet to the control plane where it is printed in a tcpdump like format.
`ctx` and `level` are the same as in `pcn_log`.
This feature is only designed for developers, so final version of services should not include this.
`polycube-tools <https://github.com/mauriciovasquezbernal/polycube-tools>`_) must be installed and `polycube` cmake must be configured with `cmake -DHAVE_POLYCUBE_TOOLS=ON ..`

Usage example:

::

  pcn_pkt_log(ctx, LOG_DEBUG);


4. Writing the control plane
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Injecting the eBPF datapath code
********************************

The user only has to pass the eBPF datapath code to the `polycube::service::Cube` base class, this will perform all the work needed for injecting it.

Adding and removing ports
*************************

The class implementing the Cube has two methods, `add_port` and `remove_port`, these methods are automatically called by the infrastructure.
Within these functions, there must be a call to `Cube::add_port()` and `Cube::remove_port()` respectively, these functions allow the infrastructure to perform all the required work for adding or removing the port.

The ``add_port`` function receives a `PortsSchema` object with the initial port configuration, and should return also a `PortsSchema` object with the configuration applied. This is the same strategy used in implementing the control api. (See below)

Accessing eBPF maps
*******************

The `polycube::service::Cube` base class provides the `get_[raw, array, percpuarray, hash, percpuhash]_table()` methods that allows to get a reference to an eBPF map.

The `RawTable` is intended to access the memory of the maps without any formatting, the user should pass pointers to key and/or value buffers.
It provides the following API:

- `void set(const void *key, const void *value)`
- `void get(const void *key, void *value)`
- `void remove(const void*key)`

The `ArrayTable` and `PercpuArrayTable` are intended to handle array like maps, this class is templated on the value type.
The `ArrayTable` provides the following API:

- `void set(const uint32_t &key, const ValueType &value)`
- `ValueType get(const uint32_t &key)`
- `std::vector<std::pair<uint32_t, std::vector<ValueType>>> get_all()`

The the `PercpuArrayTable` provides:
- `std::vector<ValueType> get(const uint32_t &key)`
- `std::vector<std::pair<uint32_t, std::vector<ValueType>>> get_all()`
- `void set(const uint32_t &key, const std::vector<ValueType> &value)`
- `void set(const uint32_t &key, const ValueType &value)`: Set value in all CPUs.

The `HashTable` and `PercpuHashTable` are intended to handle hash like maps, this class is templated on the key and value type.
The `HashTable` provides the following API:
- `ValueType get(const KeyType &key)`
- `std::vector<std::pair<KeyType, ValueType>> get_all()`
- `void set(const KeyType &key, const ValueType &value)`
- `void remove(const KeyType &key)`
- `void remove_all()`

The `PercpuHashTable` exposes:
- `std::vector<ValueType> get(const KeyType &key)`
- `std::vector<std::pair<KeyType, std::vector<ValueType>>> get_all()`
- `void set(const KeyType &key, const std::vector<ValueType> &value)`
- `void set(const KeyType &key, const ValueType &value)`: Set value in all CPUs.
- `void remove(const KeyType &key)`
- `void remove_all()`


In order to have an idea of how to implement this, take at look at the already implemented services, :doc:`router <services/pcn-router/router>` and :doc:`firewall <services/pcn-firewall/firewall>` are good examples.

Implementing the control path
*****************************

Handling PacketIn events
++++++++++++++++++++++++

If the service is intended to receive data packets in the control path (using an approach that is commonly said _slow path_), you should implement the logic that handles those packets in the `packet_in` function, it receives:
- **port**: a reference to the ingress port of the packet
- **md**: the metadata associated to this packet
    - **reason** and **metadata**: values used by the datapath when sending the packet to the control path through the `pcn_pkt_controller` functions.
- **packet**: an array containing the packet's bytes.

Generating PacketOut events
+++++++++++++++++++++++++++

The `Port` class contains the `send_packet_out(EthernetII &packet, bool recirculate = false)` method that allows to inject packets into the datapath, the recirculate parameter allows to specify if the packet should be sent out of the port (`recirculate = false`) or received through the port (`recirculate = true`).

A reference to a port can be got using the `get_port` function of the Cube base class.

Implementing the control api
****************************

The `src/{service-name}Api.cpp` file contains a stub implementation for the control api methods that create, update, read or remove resources within a service instance.
These methods receive and/or return objects representing the different datatypes defined in the `model` folder. Take at look at the header files in order to understand what functions are implemented in those classes.


Debugging and logging in the control plane
******************************************

In order to avoid the use of custom `printf` or similar primitives, which makes the code difficult to debug, polycube includes a logging system with a proper class.

Usage example:

::

  logger()->info("Connected port {0}", port_name);


Some hints for programmers
--------------------------

We apologize for this section being just an unstructured list of suggestions; better documentation will be created soon.

Install the provided git-hooks
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

An interesting feature that git provides is the possibility to run custom scripts (called `Git Hooks`) before or after events such as: commit, push, and receive.
Git hook scripts are useful for identifying simple issues before submission to code review. We run our hooks on every commit to automatically point out issues in code such as missing semicolons, trailing whitespace, and debug statements.

To solve these issues and benefit from this feature, we use [pre-commit](https://pre-commit.com/), a framework for managing and maintaining multi-language pre-commit hooks.

The `.pre-commit-config.yaml` configuration file is already available under the root folder of this repo but before you can run hooks, you need to have the pre-commit package manager installed. You can install it using pip:

::

  sudo apt-get install python-pip -y
  pip install pre-commit

After that, run pre-commit install (under the project root folder) to install pre-commit into your git hooks. pre-commit will now run on every commit.

::

  pre-commit install

How to write a test
^^^^^^^^^^^^^^^^^^^

The following is a brief guideline about how to write tests for services. Please remember such tests are invoked by a more generic script that tries to execute all tests for all services and provide global results.

1. tests are placed under `pcn-servicename\test` folder (and its first level of subfolders).
E.g. `pcn-bridge\test` and `pcn-bridge\test\vlan` are valid folders.

2. tests name begins with `test*`

3. tests scripts must be executable (`chmod +x test.sh`)

4. never launch `polycubed`: polycubed is launched by the upper script, not in the script itself

5. exit on error: script should exit when a command fails (`set -e`)

6. tests must terminate in a fixed maximum time, no `read` or long `sleep` allowed

7. tests **must** exit with a **clean environment**: all `namespaces`, `links`, `interfaces`, `cubes` created inside the script must be destroyed when script returns.
In order to do that use a `function cleanup` and set `trap cleanup EXIT` to be sure cleanup function gets always executed (also if an error is encountered, and the script fails).

8. consider that when `set -e` is enabled in your script, and you want to check if, for instance, a `ping` or `curl` command succeeds, this check is implicitly done by the returning value of the command itself.
So `ping 10.0.0.1 -c 2 -w 4` makes your script succeed if ping works, and make your script fail if it doesn't.

9. if the test `succeded` it returns `0`, otherwise returns `non-zero-value` (this is the standard behavior).
In order to check a single test result, use `echo $?` after script execution to read return value.

Please refer to existing examples (E.g. [services/pcn-helloworld/test/test1.sh](services/pcn-helloworld/test/test1.sh))


Optimizing the compilation time of dataplane eBPF programs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The compilation time of dataplane eBPF programs is an important parameter in services that are composed by many eBPF programs and/or services that use the reloading capability, as this affect the time needed to apply your changes in the data path.

Unfortunately, including large headers in the datapath code has a noticeable impact on the compilation time. Hence, in some cases it is better to copy-paste some elements (struct, macro, function, etc) and definitions to avoid including the whole file, or move to more specific headers instead of including everything from the root headers folder.

The `pcn-firewall` service is an example of service that has been optimized in this way, decreasing the dynamic loading time from tens of seconds to a few seconds.

The compilation time can be measured by enabling the `LOG_INJECTION_TIME` flag in `polycubed/src/cube.h`.


Additional hints
^^^^^^^^^^^^^^^^

1. **Creating multiple data plane programs**. If possible, it would be nice to create a single dataplane program, and enabling/disabling some portions using conditional compilation macros.

2. **Coding Style**: The `scripts/check-style.py` uses `clang-format` to check the code style.
This tool has to be executed from the root folder.
A list of files or folders to check is received as argument; the entire source code is checked when no parameters are passed.
The `--fix` option will automatically fix the code style instead of simply checking

3. **Trailing white spaces**: Trailing white spaces could generate some git noise.
Any decent text editor has an option to remove them automatically, it is a good idea to enable it.
Please notice that running `clang-format` will remove them automatically.
**NB**: If you are using our `pre-commit git hooks`, you do not need to remove the trailing whitespaces manually, they will be removed automatically at every commit.
If you want to remove them manually you can use execute the following commands in the polycube root folder, please note that this will remove trailing whitespaces of all files.
```
find . -type f -name '*.h' -exec sed -i 's/[ \t]*$//' {} \+
find . -type f -name '*.cc' -exec sed -i 's/[ \t]*$//' {} \+
find . -type f -name '*.cpp' -exec sed -i 's/[ \t]*$//' {} \+
```

4. **Debugging using bpftool**: In some cases bpftool could be useful to inspect running eBPF programs and show, dump, modify maps values.

::

  #install
  sudo apt install binutils-dev libreadline-dev
  git clone https://github.com/torvalds/linux
  cd linux/tools
  make bpf
  cd bpf
  sudo make install

  #usage
  sudo bpftool

Continuous Integration
^^^^^^^^^^^^^^^^^^^^^^

A continuous integration system based on `jenkins` is set-up for this project.

`build` pipeline is in charge to re-build the whole project from scratch at each commit.
`testing` pipeline is in charge to run all repo system tests at each commit.

pipelines results are shown in `README` top buttons `build` and `testing`.

In order to get more info about tests results, click on such buttons to get redirected to jenkins environment, then click the build number to get detailed info about a specific commit.
`testing/build` results can be read opening the tab `Console Output`.

What to do when *build failing* ?
*********************************

Click in build failing button, go to jenkins control panel, open the build number is failing, open console output in order to understand where build is failing.

What to do when *testing failing* ?
***********************************

Click in testing failing button, go to jenkins control panel, open the build number is failing, open console output in order to understand where tests are failing. At the end of the output there is a summary with tests passing/failing.
