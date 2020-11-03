# TCP 1 testing corner cases 62 63rd 64th 65th rule matched.

source "${BASH_SOURCE%/*}/../helpers.bash"

BATCH_FILE="/tmp/batch.json"
batch='{"rules":['

function fwsetup {
  polycubectl firewall add fw loglevel=DEBUG
  polycubectl attach fw veth1
  polycubectl firewall fw chain INGRESS set default=DROP
  polycubectl firewall fw chain EGRESS set default=DROP
}

function fwcleanup {
  set +e
  polycubectl firewall del fw
  #Make sure netcat is not pending.
  sudo pkill -SIGTERM netcat
  rm -f $BATCH_FILE
  delete_veth 2
}
trap fwcleanup EXIT

echo -e '\nTest TCP 1 \n'
set -e
set -x

create_veth 2

fwsetup

set +x
batch='{"rules":['

#INGRESS CHAIN
#dumb rules
for i in `seq 0 61`;
do
  batch=${batch}"{'operation': 'append', 'src': '10.1.$((i%2)).$((i%255))/31', 'dst': '10.1.$((i%2)).$((i%255))', 'l4proto': 'TCP', 'sport': $i, 'dport': $i, 'tcpflags': 'SYN', 'action': 'DROP'},"
done

#matched rules
#SYN
batch=${batch}"{'operation': 'insert', 'id': 62, 'src': '10.0.0.1', 'dst': '10.0.0.2', 'l4proto': 'TCP', 'dport': 60123, 'tcpflags': 'SYN, !ACK, !RST, !FIN','action': 'ACCEPT'},"
#ACK
batch=${batch}"{'operation': 'insert', 'id': 63, 'src': '10.0.0.1', 'dst': '10.0.0.2', 'l4proto': 'TCP', 'dport': 60123, 'tcpflags': '!SYN, ACK, !RST, !FIN','action': 'ACCEPT'},"
#FIN or FIN, ACK
batch=${batch}"{'operation': 'insert', 'id': 64, 'src': '10.0.0.1', 'dst': '10.0.0.2', 'l4proto': 'TCP', 'dport': 60123, 'tcpflags': '!SYN, !RST, FIN','action': 'ACCEPT'},"

#dumb rules
for i in `seq 65 128`;
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
#SYN, Ack
batch=${batch}"{'operation': 'insert', 'id': 63, 'src': '10.0.0.2', 'dst': '10.0.0.1', 'l4proto': 'TCP', 'sport': 60123, 'tcpflags': 'SYN, ACK, !RST', 'action': 'ACCEPT'},"
#ACK
batch=${batch}"{'operation': 'insert', 'id': 64, 'src': '10.0.0.2', 'dst': '10.0.0.1', 'l4proto': 'TCP', 'sport': 60123, 'tcpflags': 'ACK, !SYN', 'action': 'ACCEPT'},"
#FIN Or Fin, Ack
batch=${batch}"{'operation': 'insert', 'id': 65, 'src': '10.0.0.2', 'dst': '10.0.0.1', 'l4proto': 'TCP', 'sport': 60123, 'tcpflags': 'FIN, !SYN', 'action': 'ACCEPT'},"

#dumb rules
for i in `seq 66 129`;
do
  batch=${batch}"{'operation': 'append', 'src': '11.1.0.$((i%255))', 'dst': '11.1.0.$((i%255))/16', 'l4proto': 'TCP', 'sport': $i, 'dport': $i, 'tcpflags': '!ACK', 'action': 'DROP'},"
done

batch=${batch}"]}"
echo "$batch" > $BATCH_FILE
polycubectl firewall fw chain EGRESS batch rules=<$BATCH_FILE

#listen and connect
set +x
sudo netcat -l 60123&
sleep 5
sudo ip netns exec ns1 netcat -nvz 10.0.0.2 60123
