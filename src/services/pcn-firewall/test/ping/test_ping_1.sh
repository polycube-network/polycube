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

TYPE="TC"

if [ -n "$1" ]; then
  TYPE=$1
fi

create_veth 2

polycubectl firewall add fw type=$TYPE loglevel=OFF
polycubectl attach fw veth1
polycubectl firewall fw set interactive=false

#INGRESS CHAIN
#dumb rules
for i in `seq 0 62`;
do
polycubectl firewall fw chain INGRESS rule add $i src=10.1.$((i%2)).$((i%255))/31 dst=10.1.$((i%2)).$((i%255)) l4proto=TCP sport=$i dport=$i tcpflags='SYN' action=DROP
done

#matched rules
polycubectl firewall fw chain INGRESS rule add 63 src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=FORWARD

#dumb rules
for i in `seq 64 128`;
do
polycubectl firewall fw chain INGRESS rule add $i src=11.1.0.$((i%255)) dst=11.1.0.$((i%255)) l4proto=TCP sport=$i dport=$i action=DROP
done

polycubectl firewall fw chain INGRESS apply-rules

#EGRESS CHAIN
#dumb rules
for i in `seq 0 63`;
do
polycubectl firewall fw chain EGRESS rule add $i src=10.1.$((i%2)).$((i%255))/32 dst=10.1.$((i%2)).$((i%255)) l4proto=TCP sport=$i dport=$i tcpflags='!SYN' action=DROP
done

#matched rules
polycubectl firewall fw chain EGRESS rule add 64 src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=FORWARD

#dumb rules
for i in `seq 65 129`;
do
polycubectl firewall fw chain EGRESS rule add $i src=11.1.0.$((i%255)) dst=11.1.0.$((i%255))/16 l4proto=TCP sport=$i dport=$i tcpflags='!ACK' action=DROP
done

polycubectl firewall fw chain EGRESS apply-rules

#ping
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1
