# PING 5 testing don't care rules. + chain update

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

echo -e '\nTest TCP 5 \n'
set -e
set -x

create_veth 2

polycubectl firewall add fw loglevel=OFF
polycubectl attach fw veth1
set +x
batch='{"rules":['


#INGRESS CHAIN
#dumb rules
for i in `seq 0 100`;
do
  batch=${batch}"{'operation': 'append', 'src': '10.1.$((i%2)).$((i%255))/31', 'dst': '10.1.$((i%2)).$((i%255))', 'l4proto': 'TCP', 'sport': $i, 'dport': $i, 'tcpflags': 'SYN', 'action': 'DROP'},"
done

#matched rules
#DON'T CARE
batch=${batch}"{'operation': 'insert', 'id': 101, 'action': 'FORWARD'},"

#dumb rules
for i in `seq 102 200`;
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
#DON'T CARE
batch=${batch}"{'operation': 'append', 'id': 63, 'action': 'FORWARD'},"

#dumb rules
for i in `seq 64 128`;
do
  batch=${batch}"{'operation': 'append', 'src': '10.1.0.$((i%255))', 'dst': '10.1.0.$((i%255))/16', 'l4proto': 'TCP', 'sport': $i, 'dport': $i, 'tcpflags': '!ACK', 'action': 'DROP'},"
done

batch=${batch}"]}"
echo "$batch" > $BATCH_FILE
set -x
polycubectl firewall fw chain EGRESS batch rules=<$BATCH_FILE

#listen and connect
set +x
sudo netcat -l 60123&
sleep 2
sudo ip netns exec ns1 netcat -nvz 10.0.0.2 60123

