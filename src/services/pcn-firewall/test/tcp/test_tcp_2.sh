# TCP 2 testing corner cases 126th and 128th rule matched.

source "${BASH_SOURCE%/*}/../helpers.bash"

function fwcleanup {
  set +e
  polycubectl firewall del fw
  #Make sure netcat is not pending.
  sudo pkill -SIGTERM netcat
  delete_veth 2
}
trap fwcleanup EXIT

echo -e '\nTest TCP 2 \n'
set -e
set -x

create_veth 2

polycubectl firewall add fw
polycubectl firewall fw set loglevel=DEBUG
polycubectl firewall fw ports add fw-p1
polycubectl firewall fw ports add fw-p2
polycubectl firewall fw ports fw-p1 set peer=veth1
polycubectl firewall fw ports fw-p2 set peer=veth2

polycubectl firewall fw set interactive=false

#INGRESS CHAIN
#dumb rules
for i in `seq 0 125`;
do
polycubectl firewall fw chain INGRESS rule add $i src=10.1.$((i%2)).$((i%255))/31 dst=10.1.$((i%2)).$((i%255)) l4proto=TCP sport=$i dport=$i tcpflags='SYN' action=DROP
done

#matched rules
#SYN
polycubectl firewall fw chain INGRESS rule add 126 action=FORWARD src=10.0.0.1 dst=10.0.0.2 l4proto=TCP dport=60123 tcpflags='SYN, !ACK, !RST, !FIN'
#ACK
polycubectl firewall fw chain INGRESS rule  add 127 action=FORWARD src=10.0.0.1 dst=10.0.0.2 l4proto=TCP dport=60123 tcpflags='!SYN, ACK, !RST, !FIN'
#FIN or FIN, ACK
polycubectl firewall fw chain INGRESS rule  add 128 action=FORWARD src=10.0.0.1 dst=10.0.0.2 l4proto=TCP dport=60123 tcpflags='!SYN, !RST, FIN'

#dumb rules
for i in `seq 129 512`;
do
polycubectl firewall fw chain INGRESS rule add $i src=11.1.0.$((i%255)) dst=11.1.0.$((i%255)) l4proto=TCP sport=$i dport=$i action=DROP
done

polycubectl firewall fw chain INGRESS apply-rules

#EGRESS CHAIN
#dumb rules
for i in `seq 0 126`;
do
polycubectl firewall fw chain EGRESS rule add $i src=10.1.$((i%2)).$((i%255))/32 dst=10.1.$((i%2)).$((i%255)) l4proto=TCP sport=$i dport=$i tcpflags='!SYN' action=DROP
done

#matched rules
#SYN, Ack
polycubectl firewall fw chain EGRESS rule add 127 action=FORWARD l4proto=TCP src=10.0.0.2 dst=10.0.0.1 sport=60123 tcpflags='SYN, ACK, !RST'
#ACK
polycubectl firewall fw chain EGRESS rule add 128 action=FORWARD l4proto=TCP src=10.0.0.2 dst=10.0.0.1 sport=60123 tcpflags='ACK, !SYN'
#FIN Or Fin, Ack
polycubectl firewall fw chain EGRESS rule add 129 action=FORWARD l4proto=TCP src=10.0.0.2 dst=10.0.0.1 sport=60123 tcpflags='FIN, !SYN'

#dumb rules
for i in `seq 130 260`;
do
polycubectl firewall fw chain EGRESS rule add $i src=11.1.0.$((i%255)) dst=11.1.0.$((i%255))/16 l4proto=TCP sport=$i dport=$i tcpflags='!ACK' action=DROP
done

polycubectl firewall fw chain EGRESS apply-rules

#listen and connect
set +x
sudo ip netns exec ns2 netcat -l 60123&
sleep 2
sudo ip netns exec ns1 netcat -w 3 -nvz 10.0.0.2 60123&

