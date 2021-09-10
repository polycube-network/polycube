# Writing the control plane


## Handling eBPF programs


A cube is composed by 0 or more eBPF programs, the master program is the first that receives the packets to be processed.

The ``polycube::service::Cube`` and the ``polycube::service::TransparentCube`` classes provide wrapper to manage eBPF programs, it allows to load, unload and reload them.
The constructor of such classes receives two ``std::vector<std::string>`` that contains the code for the ingress and egress chain programs, those programs will be compiled and loaded when the cube is created, this is possible to create a ``Cube`` without any program and add them later on.

The following functions allow to reload, add and delete eBPF programs.

  - void reload(const std::string &code, int index, ProgramType type)
  - int add_program(const std::string &code, int index, ProgramType type)
  - void del_program(int index, ProgramType type)

Where:

  - index: position of the program
  - type: if the program is on the ``INGRESS`` or ``EGRESS`` chain.


## Adding and removing ports


The main service class has two methods, ``addPorts`` and ``removePorts``, these methods are automatically called by the infrastructure.
Within these functions, there must be a call to ``Cube::add_port()`` and ``Cube::remove_port()`` respectively, these functions allow the infrastructure to perform all the required work for adding or removing the port.

The ``Cube::add_port`` function receives a ``PortsJsonObject`` object with the initial port configuration.

The stub generated already include those functions, so in general the developer does not have to worry about it.

## eBPF maps API


The ``polycube::service::BaseCube`` base class provides the ``get_{raw, array, percpuarray, hash, percpuhash}_table()`` methods that allows to get a reference to an eBPF map.

The ``RawTable`` is intended to access the memory of the maps without any formatting, the user should pass pointers to key and/or value buffers.
It provides the following API:

  - `void set(const void* key, const void* value)`
  - `void get(const void* key, void * value)`
  - `void remove(const void* key)`

The ``ArrayTable`` and ``PercpuArrayTable`` are intended to handle array like maps, this class is templated on the value type.

The  ``ArrayTable`` provides the following API:

  - `void set(const uint32_t &key, const ValueType &value)`
  - `ValueType get(const uint32_t &key)`
  - `std::vector<std::pair<uint32_t, std::vector<ValueType>>> get_all()`

The ``PercpuArrayTable`` provides:

  - `std::vector\<ValueType> get(const uint32_t &key)`
  - `std::vector<std::pair<uint32_t, std::vector<ValueType>>> get_all()`
  - `void set(const uint32_t &key, const std::vector<ValueType> &value)`
  - `void set(const uint32_t &key, const ValueType &value)`: Set value in all CPUs.

The ``HashTable`` and ``PercpuHashTable`` are intended to handle hash like maps, this class is templated on the key and value type.
The ``HashTable`` provides the following API:

  - `ValueType get(const KeyType &key)`
  - `std::vector<std::pair<KeyType, ValueType>> get_all()`
  - `void set(const KeyType &key, const ValueType &value)`
  - `void remove(const KeyType &key)`
  - `void remove_all()`

The ``PercpuHashTable`` exposes:

  - `std::vector<ValueType> get(const KeyType &key)`
  - `std::vector<std::pair<KeyType, std::vector<ValueType>>> get_all()`
  - `void set(const KeyType &key, const std::vector<ValueType> &value)`
  - `void set(const KeyType &key, const ValueType &value)**`: Set value in all CPUs.
  - `void remove(const KeyType &key)`
  - `void remove_all()`


In order to have an idea of how to implement this, take at look at the already implemented services, :scm_web:`router <src/services/pcn-router>` and :scm_web:`firewall <src/services/pcn-firewall>` are good examples.


## Implementing the control path


### Handling PacketIn events


If the service is intended to receive data packets in the control path (using an approach that is commonly said *slow path*), you should implement the logic that handles those packets in the `packet_in` function, it receives:
  - **port**: a reference to the ingress port of the packet
  - **md**: the metadata associated to this packet
  - **reason** and **metadata**: values used by the datapath when sending the packet to the control path through the ``pcn_pkt_controller()`` functions.
  - **packet**: an array containing the packet's bytes.


### Generating PacketOut events


The ``Port`` class contains the ``send_packet_out(EthernetII &packet, bool recirculate = false)`` method that allows to inject packets into the datapath.
The ``recirculate`` parameter specifies if the packet should be sent out of the port (``recirculate = false``) or received through the port (``recirculate = true``).

Only in shadow services the ``Port`` class contains the ``send_packet_ns(EthernetII &packet)`` method that allows to send packets into the service namespace.

A reference to a port can be got using the ``get_port()`` function of the Cube base class.


## Debugging the control plane

See how to debug by [logging in the control plane](./debugging.md#logging-the-control-plane).
