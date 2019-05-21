# PING 7 testing empty rules, nrRules <=63

source "${BASH_SOURCE%/*}/../helpers.bash"

function fwcleanup {
  set +e
  polycubectl firewall del fw
  delete_veth 2
}
trap fwcleanup EXIT

echo -e '\nTest 7 \n'
set -e
set -x

create_veth 2

polycubectl firewall add fw loglevel=OFF
polycubectl attach fw veth1

#INGRESS CHAIN
#matched rules
polycubectl firewall fw chain INGRESS rule add 0 src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=FORWARD

#EGRESS CHAIN
#matched rules
polycubectl firewall fw chain EGRESS rule add 0 src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=FORWARD

#ping
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5
sudo ping 10.0.0.1 -c 2 -i 0.5
