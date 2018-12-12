# PING testing rule appending

source "${BASH_SOURCE%/*}/../helpers.bash"

function fwcleanup {
  set +e
  polycubectl firewall del fw
  delete_veth 2
}
trap fwcleanup EXIT

echo -e '\nTest 1 \n'
set -e
set -x

create_veth 2

polycubectl firewall add fw
polycubectl firewall fw ports add fw-p1
polycubectl firewall fw ports add fw-p2
polycubectl firewall fw ports fw-p1 set peer=veth1
polycubectl firewall fw ports fw-p2 set peer=veth2


polycubectl firewall fw chain INGRESS append src=10.0.0.0/8 action=DROP
polycubectl firewall fw chain EGRESS append src=10.0.0.0/8 action=DROP

polycubectl firewall fw chain INGRESS append src=10.0.0.1 action=FORWARD
polycubectl firewall fw chain EGRESS append src=10.0.0.1 action=FORWARD

polycubectl firewall fw chain INGRESS append src=10.0.0.2 action=FORWARD
polycubectl firewall fw chain EGRESS append src=10.0.0.2 action=FORWARD

#ping
test_fail sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1
