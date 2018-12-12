#!/bin/bash

set -e
set -x

"${BASH_SOURCE%/*}/test1_3.sh" XDP_SKB
