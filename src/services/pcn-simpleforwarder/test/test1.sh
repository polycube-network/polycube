# Testing simplest configuration

source "${BASH_SOURCE%/*}/helpers.bash"

function sfcleanup {
  set +e
  polycubectl simpleforwarder del sf
  delete_veth 2
}
trap sfcleanup EXIT

echo -e '\nTest 1 \n'
set -e
set -x

create_veth 2

TYPE="TC"

if [ -n "$1" ]; then
  TYPE=$1
fi

polycubectl simpleforwarder add sf type=$TYPE
polycubectl simpleforwarder sf set loglevel=DEBUG
polycubectl simpleforwarder sf ports add sf-p1
polycubectl simpleforwarder sf ports add sf-p2
polycubectl simpleforwarder sf ports sf-p1 set peer=veth1
polycubectl simpleforwarder sf ports sf-p2 set peer=veth2

polycubectl simpleforwarder sf actions add sf-p1 action=FORWARD outport=sf-p2
polycubectl simpleforwarder sf actions add sf-p2 action=FORWARD outport=sf-p1

#ping
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -w 2
