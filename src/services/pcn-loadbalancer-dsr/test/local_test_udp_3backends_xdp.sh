#!/bin/bash

set -e
set -x

"${BASH_SOURCE%/*}/test_udp_3backends.sh" XDP_SKB
