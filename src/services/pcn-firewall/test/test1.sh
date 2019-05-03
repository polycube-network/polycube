# PING 1 testing corner cases 63rd and 64th rule matched.

source "${BASH_SOURCE%/*}/helpers.bash"

function fwcleanup {
  set +e
  polycubectl firewall del fw
  delete_veth_pair
}
trap fwcleanup EXIT

echo -e '\nTest 1 \n'
set -e
set -x

setup_veth_pair

polycubectl firewall add fw loglevel=DEBUG
polycubectl attach fw veth1

#matched rules
polycubectl firewall fw chain INGRESS rule add 0 src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=FORWARD

#EGRESS CHAIN
polycubectl firewall fw chain EGRESS rule add 0 src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=FORWARD

#ping
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1
