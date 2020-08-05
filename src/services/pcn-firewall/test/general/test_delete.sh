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

polycubectl firewall add fw loglevel=DEBUG
polycubectl attach fw veth1
polycubectl firewall fw chain INGRESS set default=DROP
polycubectl firewall fw chain EGRESS set default=DROP

#matched rules
polycubectl firewall fw chain INGRESS append src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=FORWARD

#EGRESS CHAIN
polycubectl firewall fw chain EGRESS append src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=FORWARD

#ping
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1

polycubectl firewall fw chain EGRESS del rule 0

test_fail sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1


polycubectl firewall fw chain EGRESS append src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=FORWARD

polycubectl fw chain EGRESS show

sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1


polycubectl firewall fw chain EGRESS append src=10.0.0.0/24 dst=10.0.0.1/32 l4proto=ICMP action=FORWARD

polycubectl fw chain EGRESS show

sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1



polycubectl firewall fw chain EGRESS del rule 1

polycubectl fw chain EGRESS show

sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1


polycubectl firewall fw chain EGRESS del rule 0

polycubectl fw chain EGRESS show

test_fail sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1



polycubectl firewall fw chain EGRESS append src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=FORWARD

polycubectl fw chain EGRESS show

sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1


polycubectl firewall fw chain EGRESS append src=10.0.0.0/24 dst=10.0.0.1/32 l4proto=ICMP action=FORWARD

polycubectl fw chain EGRESS show

sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1



polycubectl firewall fw chain EGRESS del rule 0

polycubectl fw chain EGRESS show

sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1


polycubectl firewall fw chain EGRESS del rule 0

polycubectl fw chain EGRESS show

test_fail sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1



