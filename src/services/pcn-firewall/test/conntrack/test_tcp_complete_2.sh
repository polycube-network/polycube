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

echo "TCP Conntrack Test (ns1->ns2) [No automatic forward][Interactive mode]"

create_veth 2

polycubectl firewall add fw
polycubectl firewall fw set loglevel=TRACE
polycubectl firewall fw ports add fw-p1
polycubectl firewall fw ports add fw-p2
polycubectl firewall fw ports fw-p1 set peer=veth1
polycubectl firewall fw ports fw-p2 set peer=veth2

# Allowing connections to be started only from NS2 to NS1
polycubectl firewall fw chain EGRESS append l4proto=TCP conntrack=ESTABLISHED action=FORWARD > /dev/null
polycubectl firewall fw chain EGRESS append conntrack=INVALID action=DROP > /dev/null

polycubectl firewall fw chain INGRESS append l4proto=TCP conntrack=NEW action=FORWARD > /dev/null
polycubectl firewall fw chain INGRESS append l4proto=TCP conntrack=ESTABLISHED action=FORWARD > /dev/null
polycubectl firewall fw chain INGRESS append conntrack=INVALID action=DROP > /dev/null

#listen and connect
set -e

echo "Starting netcat server"
sudo ip netns exec ns2 netcat -l 60123&
sleep 3

echo "Starting netcat client"
sudo ip netns exec ns1 nc -w 3 -nvz 10.0.0.2 60123

echo "(1) Checking that netcat client performed the connection."
if [[ $? != 0 ]]; then
  echo "Test failed (1)"
  exit 1
fi

sleep 2
echo "(2) Checking that netcat server has been gracefully closed."
if ps kill -0 $! >/dev/null 2>&1; then
    echo "Test failed (2)"
  exit 1
fi

echo "Test PASSED"
exit 0
