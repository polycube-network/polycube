#!/bin/bash

set -e
set -x

"${BASH_SOURCE%/*}/test2_10.sh" XDP_SKB
