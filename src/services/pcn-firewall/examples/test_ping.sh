#!/bin/bash

# test ping between veth1 and veth2

sudo ip netns exec ns1 ping 10.0.0.2 -c 2
sudo ip netns exec ns2 ping 10.0.0.1 -c 2
