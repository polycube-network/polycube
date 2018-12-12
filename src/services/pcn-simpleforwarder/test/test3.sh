# Testing getters and setters

source "${BASH_SOURCE%/*}/helpers.bash"

function sfcleanup {
  set +e
  polycubectl simpleforwarder del sf
  delete_veth 2
}
trap sfcleanup EXIT

set -x
echo -e '\nTest 3 \n'
set -e
set -x

create_veth 2

polycubectl simpleforwarder add sf
polycubectl simpleforwarder sf set loglevel=DEBUG
polycubectl simpleforwarder sf ports add sf-p1
polycubectl simpleforwarder sf ports add sf-p2
polycubectl simpleforwarder sf ports sf-p1 set peer=veth1
polycubectl simpleforwarder sf ports sf-p2 set peer=veth2

polycubectl simpleforwarder sf actions add sf-p1 action=DROP
polycubectl simpleforwarder sf actions add sf-p2 action=DROP

polycubectl simpleforwarder sf actions sf-p1 set action=FORWARD
polycubectl simpleforwarder sf actions sf-p1 set outport=sf-p2

polycubectl simpleforwarder sf actions sf-p2 set action=FORWARD outport=sf-p1



#ping
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -w 2
