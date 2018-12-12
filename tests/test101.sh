#! /bin/bash

# delete a cube with one port

function cleanup {
  set +e
  polycubectl del helloworld hw0
  echo "FAIL"
}
trap cleanup EXIT

set -e
set -x

polycubectl helloworld add hw0
polycubectl helloworld hw0 ports add port1
polycubectl del helloworld hw0

set +x
trap - EXIT
echo "SUCCESS"