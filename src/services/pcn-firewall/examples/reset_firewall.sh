#!/bin/bash

set -x

polycubectl fw del 

polycubectl firewall add fw

polycubectl attach fw br:port1

echo "Firewall reconfigured"
