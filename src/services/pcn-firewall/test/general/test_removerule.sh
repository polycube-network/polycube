# PING testing deleting rule

source "${BASH_SOURCE%/*}/../helpers.bash"

function fwcleanup {
  set +e
  polycubectl firewall del fw
  delete_veth 2
}
trap fwcleanup EXIT

set -e
set -x

create_veth 2

polycubectl firewall add fw loglevel=DEBUG
polycubectl attach fw veth1

# For both chains, the first rule will be matched and the test will fail, but only if they are not deleted properly.

polycubectl firewall fw chain INGRESS append src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=DROP
polycubectl firewall fw chain INGRESS append src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=FORWARD

polycubectl firewall fw chain EGRESS rule add 0 src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=DROP
polycubectl firewall fw chain EGRESS rule add 1 src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=FORWARD

polycubectl firewall fw chain INGRESS rule del 0
polycubectl firewall fw chain EGRESS rule del 0

#ping
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1
