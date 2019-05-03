# PING 5 testing don't care rules. + chain update

source "${BASH_SOURCE%/*}/../helpers.bash"

function fwcleanup {
  set +e
  polycubectl firewall del fw
  #Make sure netcat is not pending.
  sudo pkill -SIGTERM netcat
  delete_veth 2
}
trap fwcleanup EXIT

echo -e '\nTest TCP 5 \n'
set -e
set -x

create_veth 2

polycubectl firewall add fw loglevel=OFF
polycubectl attach fw veth1

polycubectl firewall fw set interactive=false

#INGRESS CHAIN
#dumb rules
for i in `seq 0 100`;
do
polycubectl firewall fw chain INGRESS rule add $i src=10.1.$((i%2)).$((i%255))/31 dst=10.1.$((i%2)).$((i%255)) l4proto=TCP sport=$i dport=$i tcpflags='SYN' action=DROP
done

#matched rules
#DON'T CARE
polycubectl firewall fw chain INGRESS rule add 101 action=FORWARD

#dumb rules
for i in `seq 102 200`;
do
polycubectl firewall fw chain INGRESS rule add $i src=11.1.0.$((i%255)) dst=11.1.0.$((i%255)) l4proto=TCP sport=$i dport=$i action=DROP
done

polycubectl firewall fw chain INGRESS apply-rules

#EGRESS CHAIN
#dumb rules
for i in `seq 0 62`;
do
polycubectl firewall fw chain EGRESS rule add $i src=10.1.$((i%2)).$((i%255))/32 dst=10.1.$((i%2)).$((i%255)) l4proto=TCP sport=$i dport=$i tcpflags='!SYN' action=DROP
done

#matched rules
#DON'T CARE
polycubectl firewall fw chain EGRESS rule add 63 action=FORWARD

#dumb rules
for i in `seq 64 128`;
do
polycubectl firewall fw chain EGRESS rule add $i src=11.1.0.$((i%255)) dst=11.1.0.$((i%255))/16 l4proto=TCP sport=$i dport=$i tcpflags='!ACK' action=DROP
done

polycubectl firewall fw chain EGRESS apply-rules

#listen and connect
set +x
sudo netcat -l 60123&
sleep 2
sudo ip netns exec ns1 netcat -nvz 10.0.0.2 60123

