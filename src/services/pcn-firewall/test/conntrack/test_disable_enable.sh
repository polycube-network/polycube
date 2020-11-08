source "${BASH_SOURCE%/*}/../helpers.bash"

function fwsetup {
  polycubectl firewall add fw
  polycubectl attach fw veth1
  polycubectl firewall fw chain INGRESS set default=DROP
  polycubectl firewall fw chain EGRESS set default=DROP
}

function fwcleanup {
  set +e
  polycubectl firewall del fw
  delete_veth 2
}
trap fwcleanup EXIT

set -e

create_veth 2

fwsetup

echo "Conntrack Test - Enable and Disable"

echo "Disabling conntrack"
polycubectl firewall fw set conntrack=OFF

polycubectl firewall fw chain INGRESS append l4proto=ICMP action=ACCEPT
polycubectl firewall fw chain EGRESS append l4proto=ICMP action=ACCEPT

echo "(1) Performing PING without conntrack"
sudo  ping -c 2 -i 0.5 10.0.0.1
if [[ $? != 0 ]]; then
  echo "Test failed (1)"
  exit 1
fi

echo "Enabling conntrack"
polycubectl firewall fw set conntrack=ON

echo "Enabling automatic mode"
polycubectl firewall fw set accept-established=ON

polycubectl firewall fw chain INGRESS rule add 0 l4proto=ICMP conntrack=NEW action=DROP
polycubectl firewall fw chain EGRESS rule add 0 l4proto=ICMP conntrack=NEW action=ACCEPT

set +e
echo "(2) Sending NOT allowed NEW ICMP packet"
sudo ip netns exec ns1 ping -c 2 -i 0.5 10.0.0.2
if [[ $? == 0 ]]; then
  echo "Test failed (2)"
  exit 1
fi

echo "(3) Performing allowed PING"
sudo ping -c 2 -i 0.5 10.0.0.1
if [[ $? != 0 ]]; then
  echo "Test failed (3)"
  exit 1
fi

set -e
echo "Enabling manual mode"
polycubectl firewall fw set accept-established=OFF

polycubectl firewall fw chain INGRESS rule add 0 conntrack=NEW action=DROP
polycubectl firewall fw chain INGRESS rule add 1 conntrack=ESTABLISHED action=ACCEPT

polycubectl firewall fw chain EGRESS rule add 0 conntrack=NEW action=ACCEPT
polycubectl firewall fw chain EGRESS rule add 1 conntrack=ESTABLISHED action=ACCEPT

set +e
echo "(4) Sending NOT allowed NEW ICMP packet"
sudo ip netns exec ns1 ping -c 2 -i 0.5 10.0.0.2
if [[ $? == 0 ]]; then
  echo "Test failed (4)"
  exit 1
fi

echo "(5) Performing allowed PING"
sudo ping -c 2 -i 0.5 10.0.0.1
if [[ $? != 0 ]]; then
  echo "Test failed (5)"
  exit 1
fi

echo "Test PASSED"
exit 0
