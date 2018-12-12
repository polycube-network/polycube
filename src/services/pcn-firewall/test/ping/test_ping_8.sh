# PING 7 testing empty rules, nrRules >63

source "${BASH_SOURCE%/*}/../helpers.bash"

function fwcleanup {
  set +e
  polycubectl firewall del fw
  delete_veth 2
}
trap fwcleanup EXIT

echo -e '\nTest 8 \n'
set -e
set -x

create_veth 2

polycubectl firewall add fw
polycubectl firewall fw set loglevel=OFF
polycubectl firewall fw ports add fw-p1
polycubectl firewall fw ports add fw-p2
polycubectl firewall fw ports fw-p1 set peer=veth1
polycubectl firewall fw ports fw-p2 set peer=veth2

#INGRESS CHAIN
#matched rules
polycubectl firewall fw chain INGRESS rule add 100 src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=FORWARD

#EGRESS CHAIN
#matched rules
polycubectl firewall fw chain EGRESS rule add 100 src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=FORWARD

#ping
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5
