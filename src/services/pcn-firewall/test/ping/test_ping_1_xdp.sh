#!/bin/bash

set -e
set -x

"${BASH_SOURCE%/*}/test_ping_1.sh" XDP_SKB
