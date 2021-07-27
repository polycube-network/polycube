#!/bin/bash

set -x

polycubectl fw del 

polycubectl firewall add fw
polycubectl firewall fw chain INGRESS set default=DROP
polycubectl firewall fw chain EGRESS set default=DROP

polycubectl attach fw br:port1

echo "Firewall reconfigured"
