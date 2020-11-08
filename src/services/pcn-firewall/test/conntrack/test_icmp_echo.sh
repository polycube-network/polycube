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
set -x

create_veth 2

fwsetup

polycubectl firewall fw set accept-established=OFF

# Allowing connections to be started only from NS2 to NS1
polycubectl firewall fw chain INGRESS append conntrack=NEW action=DROP
polycubectl firewall fw chain INGRESS append conntrack=ESTABLISHED action=ACCEPT

polycubectl firewall fw chain EGRESS append conntrack=NEW action=ACCEPT
polycubectl firewall fw chain EGRESS append conntrack=ESTABLISHED action=ACCEPT


echo "ICMP Echo Conntrack Test [No automatic ACCEPT][Interactive mode]"

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
