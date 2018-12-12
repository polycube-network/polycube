#!/bin/bash

# test TCP traffic between veth1 and veth2
set -x

sudo ip netns exec ns1 nping -c 2 --tcp --source-port 1234 --dest-port 5678 10.0.0.2

sudo ip netns exec ns2 nping -c 2 --tcp --source-port 5678 --dest-port 1234 10.0.0.1
