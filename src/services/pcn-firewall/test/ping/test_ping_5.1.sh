# PING 5 testing don't care rules.
# ACHTUNG! Disable tracing (firewall.trace = false)

source "${BASH_SOURCE%/*}/../helpers.bash"

batch='{"rules":['

function fwsetup {
  polycubectl firewall add fw loglevel=DEBUG
  polycubectl attach fw veth1
  polycubectl firewall fw chain INGRESS set default=DROP
  polycubectl firewall fw chain EGRESS set default=DROP
  polycubectl firewall fw set accept-established=OFF
}

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

fwsetup

set +x
batch='{"rules":['

#INGRESS CHAIN
#dumb rules
for i in `seq 0 3499`;
do
  batch=${batch}"{'operation': 'append', 'src': '10.1.$((i%2)).$((i%255))/31', 'dst': '10.1.$((i%2)).$((i%255))', 'l4proto': 'TCP', 'sport': $i, 'dport': $i, 'tcpflags': 'SYN', 'action': 'DROP'},"
done

#matched rules
batch=${batch}"{'operation': 'append', 'action': 'ACCEPT'},"

#dumb rules
for i in `seq 3501 4000`;
do
  batch=${batch}"{'operation': 'append', 'src': '11.1.0.$((i%255))', 'dst': '11.1.0.$((i%255))', 'l4proto': 'TCP', 'sport': $i, 'dport': $i, 'action': 'DROP'},"
done

batch=${batch}"]}"
set -x
polycubectl firewall fw chain INGRESS batch rules=<<<$batch
set +x
batch='{"rules":['

#EGRESS CHAIN
#dumb rules
for i in `seq 0 3500`;
do
  batch=${batch}"{'operation': 'append', 'src': '10.1.$((i%2)).$((i%255))/32', 'dst': '10.1.$((i%2)).$((i%255))', 'l4proto': 'TCP', 'sport': $i, 'dport': $i, 'tcpflags': '!SYN', 'action': 'DROP'},"
done

#matched rules
batch=${batch}"{'operation': 'append', 'action': 'ACCEPT'},"

#dumb rules
for i in `seq 3502 4000`;
do
  batch=${batch}"{'operation': 'append', 'src': '11.1.0.$((i%255))', 'dst': '11.1.0.$((i%255))/16', 'l4proto': 'TCP', 'sport': $i, 'dport': $i, 'tcpflags': '!ACK', 'action': 'DROP'},"
done

batch=${batch}"]}"
set -x
polycubectl firewall fw chain EGRESS batch rules=<<<$batch

#pings
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1
sudo ping 10.0.0.1 -c 2 -i 0.5 -w 1
