# PING testing rule appending

source "${BASH_SOURCE%/*}/../helpers.bash"

function fwcleanup {
  set +e
  #polycubectl detach fw veth1
  polycubectl firewall del fw
  delete_veth 2
}
trap fwcleanup EXIT

echo -e '\nTest 1 \n'
set -e
set -x

create_veth 2

polycubectl firewall add fw loglevel=DEBUG
polycubectl attach fw veth1


polycubectl firewall fw chain INGRESS append src=10.0.0.0/8 action=DROP
polycubectl firewall fw chain EGRESS append src=10.0.0.0/8 action=DROP

polycubectl firewall fw chain INGRESS append src=10.0.0.1 action=FORWARD
polycubectl firewall fw chain EGRESS append src=10.0.0.1 action=FORWARD

polycubectl firewall fw chain INGRESS append src=10.0.0.2 action=FORWARD
polycubectl firewall fw chain EGRESS append src=10.0.0.2 action=FORWARD

#ping
test_fail sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1
