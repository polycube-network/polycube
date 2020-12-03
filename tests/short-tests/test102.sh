#! /bin/bash

# delete a cube with one port attached

source "${BASH_SOURCE%/*}/helpers.bash"

function cleanup {
  set +e
  polycubectl del helloworld hw0
  delete_veth 1
  echo "FAIL"
}
trap cleanup EXIT

create_veth 1

set -x
set -e

polycubectl helloworld add hw0
polycubectl helloworld hw0 ports add port1
polycubectl helloworld hw0 ports port1 set peer=veth1
polycubectl del helloworld hw0

set +x
delete_veth 1
trap - EXIT
echo "SUCCESS"