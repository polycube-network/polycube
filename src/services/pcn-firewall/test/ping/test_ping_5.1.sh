# PING 5 testing don't care rules.
# ACHTUNG! Disable tracing (firewall.trace = false)

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

echo -e '\nTest 5 \n'
set -e
set -x

create_veth 2

polycubectl firewall add fw
polycubectl attach fw veth1
set +x
batch='{"rules":['

#INGRESS CHAIN
#dumb rules
for i in `seq 0 3499`;
do
  batch=${batch}"{'operation': 'append', 'src': '10.1.$((i%2)).$((i%255))/31', 'dst': '10.1.$((i%2)).$((i%255))', 'l4proto': 'TCP', 'sport': $i, 'dport': $i, 'tcpflags': 'SYN', 'action': 'DROP'},"
done

#matched rules
batch=${batch}"{'operation': 'insert', 'id': 3500, 'action': 'FORWARD'},"

#dumb rules
for i in `seq 3501 4000`;
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
for i in `seq 0 3500`;
do
  batch=${batch}"{'operation': 'append', 'src': '10.1.$((i%2)).$((i%255))/32', 'dst': '10.1.$((i%2)).$((i%255))', 'l4proto': 'TCP', 'sport': $i, 'dport': $i, 'tcpflags': '!SYN', 'action': 'DROP'},"
done

#matched rules
batch=${batch}"{'operation': 'insert', 'id':3501, 'action': 'FORWARD'},"

#dumb rules
for i in `seq 3502 4000`;
do
  batch=${batch}"{'operation': 'append', 'src': '11.1.0.$((i%255))', 'dst': '11.1.0.$((i%255))/16', 'l4proto': 'TCP', 'sport': $i, 'dport': $i, 'tcpflags': '!ACK', 'action': 'DROP'},"
done

batch=${batch}"]}"
echo "$batch" > $BATCH_FILE
set -x
polycubectl firewall fw chain EGRESS batch rules=<$BATCH_FILE

#pings
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1
sudo ping 10.0.0.1 -c 2 -i 0.5 -w 1
