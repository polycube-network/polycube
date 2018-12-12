# Testing 4 namespaces configuration
# veth1 <--> veth2
# veth3 <--> veth4

source "${BASH_SOURCE%/*}/helpers.bash"

function sfcleanup {
  set +e
  polycubectl simpleforwarder del sf
  delete_veth 4
}
trap sfcleanup EXIT

echo -e '\nTest 2 \n'
set -e
set -x

create_veth 4

polycubectl simpleforwarder add sf
polycubectl simpleforwarder sf set loglevel=DEBUG
polycubectl simpleforwarder sf ports add sf-p1
polycubectl simpleforwarder sf ports add sf-p2
polycubectl simpleforwarder sf ports add sf-p3
polycubectl simpleforwarder sf ports add sf-p4
polycubectl simpleforwarder sf ports sf-p1 set peer=veth1
polycubectl simpleforwarder sf ports sf-p2 set peer=veth2
polycubectl simpleforwarder sf ports sf-p3 set peer=veth3
polycubectl simpleforwarder sf ports sf-p4 set peer=veth4

polycubectl simpleforwarder sf actions add sf-p1 action=FORWARD outport=sf-p2
polycubectl simpleforwarder sf actions add sf-p2 action=FORWARD outport=sf-p1

polycubectl simpleforwarder sf actions add sf-p3 action=FORWARD outport=sf-p4
polycubectl simpleforwarder sf actions add sf-p4 action=FORWARD outport=sf-p3


#ping
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -w 2
sudo ip netns exec ns4 ping 10.0.0.3 -c 2 -w 2
