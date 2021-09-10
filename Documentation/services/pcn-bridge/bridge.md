# Bridge

## Features


- Support for VLANs
- Support for access and trunk mode for the ports.
- Support for CSTP (Common-STP) and PVSTP (Per Vlan-STP)

## Limitations


- Currently it does not accept all vlans on a trunk port

## How to use


Create instances and ports

```
# create the instance
polycubectl bridge add br1

# add ports
polycubectl br1 ports add p1
```

VLAN configuration

```
# change VLAN in access mode
polycubectl br1 ports p1 access set vlanid=2
    
# change port mode (access/trunk)
polycubectl br1 ports p1 set mode=trunk
    
# add an allowed vlan in a trunk port
polycubectl br1 ports p1 trunk allowed add 10

# change native vlan in a trunk port
polycubectl br1 ports p1 trunk set native-vlan=2

# enable/disable native vlan in a trunk port
polycubectl br1 ports p1 trunk set native-vlan-enabled=false
```

Spanning Tree configuration

```
# enable/disable spanning tree protocol
polycubectl br1 set stp-enabled=true

# view active instances of STP
polycubectl br1 stp show

# view a particular instance
polycubectl br1 stp 1 show

# modify a parameter in an active instance
polycubectl br1 stp 1 set priority=28672

# view STP configuration in a port
polycubectl br1 ports p1 stp 1 show

# modify a paremeter in an active instance of a port
polycubectl br1 ports p1 stp 1 set port-priority=64
```

