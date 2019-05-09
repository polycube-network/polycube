# PING 1 testing corner cases 63rd and 64th rule matched.

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

polycubectl firewall add fw loglevel=OFF
polycubectl attach fw veth1

polycubectl firewall fw set interactive=false

#INGRESS CHAIN
#dumb rules
for i in `seq 0 1`;
do
polycubectl firewall fw chain INGRESS rule add $i src=10.1.$((i%2)).$((i%255))/31 dst=10.1.$((i%2)).$((i%255)) l4proto=TCP sport=$i dport=$i tcpflags='SYN' action=DROP
done

#matched rules
polycubectl firewall fw chain INGRESS rule add 2 src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=FORWARD

polycubectl firewall fw chain INGRESS apply-rules

#EGRESS CHAIN
#dumb rules
for i in `seq 0 1`;
do
polycubectl firewall fw chain EGRESS rule add $i src=10.1.$((i%2)).$((i%255))/32 dst=10.1.$((i%2)).$((i%255)) l4proto=TCP sport=$i dport=$i tcpflags='!SYN' action=DROP
done

#matched rules
polycubectl firewall fw chain EGRESS rule add 2 src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=FORWARD

polycubectl firewall fw chain EGRESS apply-rules

#ping
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5
