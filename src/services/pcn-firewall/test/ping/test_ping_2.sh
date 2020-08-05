# PING 2 testing corner cases 126th and 128th rule matched.

source "${BASH_SOURCE%/*}/../helpers.bash"

BATCH_FILE="/tmp/batch.json"
batch='{"rules":['

function fwcleanup {
  set +e
  polycubectl firewall del fw
  rm -f $BATCH_FILE
  delete_veth 2
}
trap fwcleanup EXIT

echo -e '\nTest 2 \n'
set -e
set -x

create_veth 2

polycubectl firewall add fw loglevel=OFF
polycubectl attach fw veth1
set +x
batch='{"rules":['

#INGRESS CHAIN
#dumb rules
for i in `seq 0 125`;
do
  batch=${batch}"{'operation': 'append', 'src': '10.1.$((i%2)).$((i%255))/31', 'dst': '10.1.$((i%2)).$((i%255))', 'l4proto': 'TCP', 'sport': $i, 'dport': $i, 'tcpflags': 'SYN', 'action': 'DROP'},"
done

#matched rules
batch=${batch}"{'operation': 'insert', 'id': 126, 'src': '10.0.0.1', 'dst': '10.0.0.2', 'l4proto': 'ICMP', 'action': 'FORWARD'},"

#dumb rules
for i in `seq 127 250`;
do
  batch=${batch}"{'operation': 'append', 'src': '11.1.0.$((i%255))', 'dst': '11.1.0.$((i%255))', 'l4proto': 'TCP', 'sport': $i, 'dport': $i, 'action': 'DROP'},"
done

batch=${batch}"]}"
echo "$batch" > $BATCH_FILE
set -x
polycubectl firewall fw chain INGRESS batch rules=<$BATCH_FILE
set +x
batch='{"rules":['


#EGRESS CHAIN
#dumb rules
for i in `seq 0 62`;
do
  batch=${batch}"{'operation': 'append', 'src': '10.1.$((i%2)).$((i%255))/32', 'dst': '10.1.$((i%2)).$((i%255))', 'l4proto': 'TCP', 'sport': $i, 'dport': $i, 'tcpflags': '!SYN', 'action': 'DROP'},"
done

#matched rules
batch=${batch}"{'operation': 'insert', 'id': 63, 'src': '10.0.0.2/32', 'dst': '10.0.0.1/32', 'l4proto': 'ICMP', 'action': 'FORWARD'},"

#dumb rules
for i in `seq 64 129`;
do
  batch=${batch}"{'operation': 'append', 'src': '11.1.0.$((i%255))', 'dst': '11.1.0.$((i%255))/16', 'l4proto': 'TCP', 'sport': $i, 'dport': $i, 'tcpflags': '!ACK', 'action': 'DROP'},"
done

batch=${batch}"]}"
echo "$batch" > $BATCH_FILE
set -x
polycubectl firewall fw chain EGRESS batch rules=<$BATCH_FILE


#ping
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5
