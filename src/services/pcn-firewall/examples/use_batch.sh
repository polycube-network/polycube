#!/bin/bash

set -x

# assume polycubed is already running
# sudo polycubed -d

# assume standard cube (br) and firewall (fw) already created and running
# ./setup_env.sh

echo 'Example using batch'

set -e
set -x

ingress='{
  "rules": [
    {"operation": "append", "l4proto":"ICMP", "src":"10.0.0.1", "dst":"10.0.0.2", "action":"accept"},
    {"operation": "append", "l4proto": "TCP", "src":"10.0.0.1", "dst":"10.0.0.2", "action":"drop"}
  ]
}'

egress='{
  "rules": [
    {"operation": "append", "l4proto":"ICMP", "src":"10.0.0.2/32", "dst":"10.0.0.1/32", "action":"accept"},
    {"operation": "append", "l4proto": "TCP", "src":"10.0.0.2", "dst":"10.0.0.1", "action":"drop"}
  ]
}'

polycubectl firewall fw chain INGRESS batch rules=<<<$ingress
polycubectl firewall fw chain EGRESS batch rules=<<<$egress

#ping
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -w 2
