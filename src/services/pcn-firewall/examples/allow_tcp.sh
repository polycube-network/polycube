#!/bin/bash

set -x

# assume polycubed is already running
# sudo polycubed -d

# assume standard cube (br) and firewall (fw) already created and running
# ./setup_cube.sh

# allow TCP traffic from/to 10.0.0.0/24

polycubectl firewall fw chain EGRESS insert l4proto=TCP src=10.0.0.0/24 dst=10.0.0.0/24 action=FORWARD

polycubectl firewall fw chain INGRESS insert l4proto=TCP src=10.0.0.0/24 dst=10.0.0.0/24 action=FORWARD

echo "Wait for the rules to be updated and launch test_tcp.sh"
