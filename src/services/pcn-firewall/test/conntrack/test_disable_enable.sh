source "${BASH_SOURCE%/*}/helpers.bash"

function fwcleanup {
  set +e
  polycubectl firewall del fw
  delete_veth 2
}
trap fwcleanup EXIT

set -e

create_veth 2

polycubectl firewall add fw
polycubectl firewall fw set loglevel=DEBUG
polycubectl firewall fw ports add fw-p1
polycubectl firewall fw ports add fw-p2
polycubectl firewall fw ports fw-p1 set peer=veth1
polycubectl firewall fw ports fw-p2 set peer=veth2

echo "Conntrack Test - Enable and Disable"

echo "Disabling conntrack"
polycubectl firewall fw set conntrack=OFF

polycubectl firewall fw chain INGRESS append l4proto=ICMP action=FORWARD
polycubectl firewall fw chain EGRESS append l4proto=ICMP action=FORWARD

echo "(1) Performing PING without conntrack"
sudo ip netns exec ns2 ping -c 2 -i 0.5 10.0.0.1
if [[ $? != 0 ]]; then
  echo "Test failed (1)"
  exit 1
fi

echo "Enabling conntrack"
polycubectl firewall fw set conntrack=ON

echo "Enabling automatic mode"
polycubectl firewall fw set accept-established=ON

polycubectl firewall fw chain INGRESS rule add 0 l4proto=ICMP conntrack=NEW action=DROP
polycubectl firewall fw chain EGRESS rule add 0 l4proto=ICMP conntrack=NEW action=FORWARD

set +e
echo "(2) Sending NOT allowed NEW ICMP packet"
sudo ip netns exec ns1 ping -c 2 -i 0.5 10.0.0.2
if [[ $? == 0 ]]; then
  echo "Test failed (2)"
  exit 1
fi

echo "(3) Performing allowed PING"
sudo ip netns exec ns2 ping -c 2 -i 0.5 10.0.0.1
if [[ $? != 0 ]]; then
  echo "Test failed (3)"
  exit 1
fi

set -e
echo "Enabling manual mode"
polycubectl firewall fw set accept-established=OFF

polycubectl firewall fw chain INGRESS rule add 0 conntrack=NEW action=DROP
polycubectl firewall fw chain INGRESS rule add 1  conntrack=ESTABLISHED action=FORWARD

polycubectl firewall fw chain EGRESS rule add 0 conntrack=NEW action=FORWARD
polycubectl firewall fw chain EGRESS rule add 1 conntrack=ESTABLISHED action=FORWARD

set +e
echo "(4) Sending NOT allowed NEW ICMP packet"
sudo ip netns exec ns1 ping -c 2 -i 0.5 10.0.0.2
if [[ $? == 0 ]]; then
  echo "Test failed (4)"
  exit 1
fi

echo "(5) Performing allowed PING"
sudo ip netns exec ns2 ping -c 2 -i 0.5 10.0.0.1
if [[ $? != 0 ]]; then
  echo "Test failed (5)"
  exit 1
fi

echo "Test PASSED"
exit 0
