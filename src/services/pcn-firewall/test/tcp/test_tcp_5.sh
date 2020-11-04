# PING 5 testing don't care rules. + chain update

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
  #Make sure netcat is not pending.
  sudo pkill -SIGTERM netcat
  delete_veth 2
}
trap fwcleanup EXIT

echo -e '\nTest TCP 5 \n'
set -e
set -x

create_veth 2

fwsetup

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
batch=${batch}"{'operation': 'insert', 'id': 101, 'action': 'ACCEPT'},"

#dumb rules
for i in `seq 102 200`;
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
for i in `seq 0 62`;
do
  batch=${batch}"{'operation': 'append', 'src': '10.1.$((i%2)).$((i%255))/32', 'dst': '10.1.$((i%2)).$((i%255))', 'l4proto': 'TCP', 'sport': $i, 'dport': $i, 'tcpflags': '!SYN', 'action': 'DROP'},"
done

#matched rules
#DON'T CARE
batch=${batch}"{'operation': 'append', 'id': 63, 'action': 'ACCEPT'},"

#dumb rules
for i in `seq 64 128`;
do
  batch=${batch}"{'operation': 'append', 'src': '10.1.0.$((i%255))', 'dst': '10.1.0.$((i%255))/16', 'l4proto': 'TCP', 'sport': $i, 'dport': $i, 'tcpflags': '!ACK', 'action': 'DROP'},"
done

batch=${batch}"]}"
set -x
polycubectl firewall fw chain EGRESS batch rules=<<<$batch

#listen and connect
set +x
sudo netcat -l 60123&
sleep 2
sudo ip netns exec ns1 netcat -nvz 10.0.0.2 60123

