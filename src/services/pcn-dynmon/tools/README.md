#  Dynmon Injector

This tool allows the creation and the manipulation of a `dynmon` cube without using the standard `polycubectl` CLI.

## Install
Some dependencies are required for this tool to run:
```
pip install -r requirements.txt
```

## Run

Running the tool:

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

Usage examples:

```
basic usage:
./dynmon_injector.py monitor_0 eno1 ../examples/packet_counter.json

setting custom ip address and port to contact the polycube daemon:
./dynmon_injector.py -a 10.0.0.1 -p 5840 monitor_0 eno1 ../examples/packet_counter.json

```

This tool creates a new `dynmon` cube with the given configuration and attaches it to the selected interface.

If the monitor already exists, the tool checks if the attached interface is the same used previously; if not, it detaches the cube from the previous interface and attaches it to the new one; then, the selected dataplane is injected.

To inspect the current configuration of the cube or to read extracted monitoring data, use the [`polycubectl`](https://polycube-network.readthedocs.io/en/latest/polycubectl/polycubectl.html) command line interface.
