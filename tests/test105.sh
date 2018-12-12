#! /bin/bash

# delete a interface that is a attached to a cube's port,
# then check that peer is automatically reset

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
set +e
sudo ip link del veth1
set -e
peer=`polycubectl helloworld hw0 ports port1 show peer`
if [ ! -z "$peer"]; then
  exit 1
fi
polycubectl del helloworld hw0

set +x
trap - EXIT
echo "SUCCESS"