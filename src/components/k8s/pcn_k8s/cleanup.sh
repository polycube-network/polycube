#!/bin/bash

ip link del dev pcn_k8s
ip link del dev pcn_vxlan

# TODO: remove masquerate rules on external interface
