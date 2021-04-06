# PING testing rule appending

source "${BASH_SOURCE%/*}/../helpers.bash"

function fwsetup {
  polycubectl firewall add fw
  polycubectl attach fw veth1
  polycubectl firewall fw chain INGRESS set default=DROP
  polycubectl firewall fw chain EGRESS set default=DROP
}

function fwcleanup {
  set +e
  polycubectl firewall del fw
  delete_veth 2
}
trap fwcleanup EXIT

echo -e '\nTest insert \n'
set -e
set -x

create_veth 2

fwsetup

# test simple insert rules
polycubectl firewall fw chain INGRESS insert src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=ACCEPT

polycubectl firewall fw chain EGRESS insert src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=ACCEPT

sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1


polycubectl firewall fw chain INGRESS insert src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=DROP

test_fail sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1


polycubectl firewall fw chain INGRESS delete src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=DROP

polycubectl firewall fw chain INGRESS show

sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1

# test insert rule in specific position
polycubectl firewall fw chain INGRESS insert id=1 src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=DROP

polycubectl firewall fw chain INGRESS show

sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1

# test wrong position
set +e
polycubectl firewall fw chain INGRESS insert id=2 src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=DROP
polycubectl firewall fw chain INGRESS insert id=5 src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=DROP
polycubectl firewall fw chain INGRESS insert id=-1 src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=DROP
set -e

polycubectl firewall fw chain INGRESS show

sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1
