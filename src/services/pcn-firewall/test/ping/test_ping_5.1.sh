# PING 5 testing don't care rules.
# ACHTUNG! Disable tracing (firewall.trace = false)

source "${BASH_SOURCE%/*}/../helpers.bash"

function fwcleanup {
  set +e
  polycubectl firewall del fw
  delete_veth 2
}
trap fwcleanup EXIT

echo -e '\nTest 5 \n'
set -e
set -x

create_veth 2

polycubectl firewall add fw
polycubectl attach fw veth1
polycubectl firewall fw set interactive=false

#INGRESS CHAIN
#dumb rules
for i in `seq 0 3499`;
do
polycubectl firewall fw chain INGRESS rule add $i src=10.1.$((i%2)).$((i%255))/31 dst=10.1.$((i%2)).$((i%255)) l4proto=TCP sport=$i dport=$i tcpflags='SYN' action=DROP
done

#matched rules
polycubectl firewall fw chain INGRESS rule add 3500 action=FORWARD

#dumb rules
for i in `seq 3501 4000`;
do
polycubectl firewall fw chain INGRESS rule add $i src=11.1.0.$((i%255)) dst=11.1.0.$((i%255)) l4proto=TCP sport=$i dport=$i action=DROP
done

polycubectl firewall fw chain INGRESS apply-rules

#EGRESS CHAIN
#dumb rules
for i in `seq 0 3500`;
do
polycubectl firewall fw chain EGRESS rule add $i src=10.1.$((i%2)).$((i%255))/32 dst=10.1.$((i%2)).$((i%255)) l4proto=TCP sport=$i dport=$i tcpflags='!SYN' action=DROP
done

#matched rules
polycubectl firewall fw chain EGRESS rule add 3501 action=FORWARD

#dumb rules
for i in `seq 3502 4000`;
do
polycubectl firewall fw chain EGRESS rule add $i src=11.1.0.$((i%255)) dst=11.1.0.$((i%255))/16 l4proto=TCP sport=$i dport=$i tcpflags='!ACK' action=DROP
done

polycubectl firewall fw chain EGRESS apply-rules


#pings
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1
sudo ping 10.0.0.1 -c 2 -i 0.5 -w 1
