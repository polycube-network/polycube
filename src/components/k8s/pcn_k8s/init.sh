#!/bin/bash

set -x
set -e

extIface=$1

iptables -P FORWARD ACCEPT

# masquerade packets ongoing on the public iface: provide internet access for pods
# TODO: be more espeficic and masquerate only packets comming from pods
iptables -t nat -A POSTROUTING -o $extIface -j MASQUERADE
