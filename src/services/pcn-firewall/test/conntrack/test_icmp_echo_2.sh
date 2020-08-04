source "${BASH_SOURCE%/*}/../helpers.bash"

function fwcleanup {
  set +e
  polycubectl firewall del fw
  delete_veth 2
}
trap fwcleanup EXIT

set -e
set -x

create_veth 2

polycubectl firewall add fw loglevel=DEBUG
polycubectl attach fw veth1

polycubectl firewall fw set accept-established=OFF

# Allowing connections to be started only from NS2 to NS1
polycubectl firewall fw chain INGRESS append conntrack=NEW action=DROP
polycubectl firewall fw chain INGRESS append conntrack=ESTABLISHED action=FORWARD

polycubectl firewall fw chain EGRESS append conntrack=NEW action=FORWARD
polycubectl firewall fw chain EGRESS append conntrack=ESTABLISHED action=FORWARD

echo "ICMP Echo Conntrack Test [No automatic forward][Transaction mode]"

set +e
echo "(1) Sending NOT allowed NEW ICMP packet"
sudo ip netns exec ns1 ping -c 2 -i 0.5 10.0.0.2
if [[ $? == 0 ]]; then
  echo "Test failed (1)"
  exit 1
fi

echo "(2) Performing allowed PING"
sudo ping -c 2 -i 0.5 10.0.0.1
if [[ $? != 0 ]]; then
  echo "Test failed (2)"
  exit 1
fi

echo "Test PASSED"
exit 0
