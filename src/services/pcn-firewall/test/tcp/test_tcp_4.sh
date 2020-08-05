# PING 4 testing many rules: 7000 each chain.

source "${BASH_SOURCE%/*}/../helpers.bash"

BATCH_FILE="/tmp/batch.json"
batch='{"rules":['

function fwcleanup {
  set +e
  polycubectl firewall del fw
  #Make sure netcat is not pending.
  sudo pkill -SIGTERM netcat
  rm -f $BATCH_FILE
  delete_veth 2
}
trap fwcleanup EXIT

echo -e '\nTest TCP 4 \n'
set -e
set -x

create_veth 2

polycubectl firewall add fw loglevel=OFF
polycubectl attach fw veth1
set +x
batch='{"rules":['

#INGRESS CHAIN
#dumb rules
for i in `seq 0 6000`;
do
  batch=${batch}"{'operation': 'append', 'src': '10.1.$((i%2)).$((i%255))/31', 'dst': '10.1.$((i%2)).$((i%255))', 'l4proto': 'TCP', 'sport': $i, 'dport': $i, 'tcpflags': 'SYN', 'action': 'DROP'},"
done

#matched rules
#SYN
batch=${batch}"{'operation': 'insert', 'id': 6001, 'src': '10.0.0.1', 'dst': '10.0.0.2', 'l4proto': 'TCP', 'dport': 60123, 'tcpflags': 'SYN, !ACK, !RST, !FIN','action': 'FORWARD'},"
#ACK
batch=${batch}"{'operation': 'insert', 'id': 6002, 'src': '10.0.0.1', 'dst': '10.0.0.2', 'l4proto': 'TCP', 'dport': 60123, 'tcpflags': '!SYN, ACK, !RST, !FIN','action': 'FORWARD'},"
#FIN or FIN, ACK
batch=${batch}"{'operation': 'insert', 'id': 6003, 'src': '10.0.0.1', 'dst': '10.0.0.2', 'l4proto': 'TCP', 'dport': 60123, 'tcpflags': '!SYN, !RST, FIN','action': 'FORWARD'},"

#dumb rules
for i in `seq 6004 6999`;
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
for i in `seq 0 6996`;
do
  batch=${batch}"{'operation': 'append', 'src': '10.1.$((i%2)).$((i%255))/32', 'dst': '10.1.$((i%2)).$((i%255))', 'l4proto': 'TCP', 'sport': $i, 'dport': $i, 'tcpflags': '!SYN', 'action': 'DROP'},"
done

#matched rules
#SYN, Ack
batch=${batch}"{'operation': 'insert', 'id': 127, 'src': '10.0.0.2', 'dst': '10.0.0.1', 'l4proto': 'TCP', 'sport': 60123, 'tcpflags': 'SYN, ACK, !RST', 'action': 'FORWARD'},"
#ACK
batch=${batch}"{'operation': 'insert', 'id': 128, 'src': '10.0.0.2', 'dst': '10.0.0.1', 'l4proto': 'TCP', 'sport': 60123, 'tcpflags': 'ACK, !SYN', 'action': 'FORWARD'},"
#FIN Or Fin, Ack
batch=${batch}"{'operation': 'insert', 'id': 129, 'src': '10.0.0.2', 'dst': '10.0.0.1', 'l4proto': 'TCP', 'sport': 60123, 'tcpflags': 'FIN, !SYN', 'action': 'FORWARD'},"

batch=${batch}"]}"
echo "$batch" > $BATCH_FILE
set -x
polycubectl firewall fw chain EGRESS batch rules=<$BATCH_FILE

#listen and connect
set +x
sudo netcat -l 60123&
sleep 2
sudo ip netns exec ns1 netcat -nvz 10.0.0.2 60123

