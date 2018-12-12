#!/bin/bash

set -e
set -x

"${BASH_SOURCE%/*}/test_ingress_egress_1.sh" XDP_SKB
