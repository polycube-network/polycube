source "${BASH_SOURCE%/*}/../helpers.bash"

function fwcleanup {
  set +e
  polycubectl firewall del fw
  #Make sure netcat is not pending.
  sudo pkill -SIGTERM netcat
  delete_veth 2
}
trap fwcleanup EXIT
set -e

echo "TCP Initial handshake Conntrack Test [Automatic forward][Interactive mode]"

create_veth 2

polycubectl firewall add fw loglevel=TRACE
polycubectl attach fw veth1

polycubectl firewall fw set accept-established=ON

# Allowing connections to be started only from NS2 to NS1
polycubectl firewall fw chain INGRESS append l4proto=ICMP action=FORWARD > /dev/null

polycubectl firewall fw chain EGRESS append l4proto=TCP conntrack=NEW action=FORWARD > /dev/null
polycubectl firewall fw chain EGRESS append l4proto=ICMP action=FORWARD > /dev/null

#listen and connect
set -e
sudo ip netns exec ns1 netcat -l 60123&
sleep 3

sudo nc -w 3 -nvz 10.0.0.1 60123

if [[ $? != 0 ]]; then
  echo "Test failed"
  exit 1
fi

echo "Test PASSED"
exit 0
