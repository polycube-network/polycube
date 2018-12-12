#!/bin/bash

set -e
set -x

"${BASH_SOURCE%/*}/test3_3s.sh" XDP_SKB
