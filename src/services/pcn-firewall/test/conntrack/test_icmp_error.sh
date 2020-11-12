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

# Allowing only ICMP errors to be freely ACCEPTed
polycubectl firewall fw chain INGRESS append l4proto=UDP action=ACCEPT
polycubectl firewall fw chain INGRESS append l4proto=ICMP conntrack=RELATED action=ACCEPT
polycubectl firewall fw chain INGRESS append l4proto=ICMP action=DROP

polycubectl firewall fw chain EGRESS append l4proto=UDP action=ACCEPT
polycubectl firewall fw chain EGRESS append l4proto=ICMP conntrack=RELATED action=ACCEPT
polycubectl firewall fw chain EGRESS append l4proto=ICMP action=DROP

set +x
echo "ICMP Error Conntrack Test [Interactive mode]"

echo "(1) Sending UDP packet that will trigger an error response"
npingOutput="$(sudo ip netns exec ns1 nping --udp -c 1 -p 50000 -g 50000 10.0.0.2)"
if [[ $npingOutput != *"Rcvd: 1"* ]]; then
  echo "Test failed (1)"
  exit 1
fi

sleep 5

echo "(2) Sending again an UDP packet that will trigger an error response"
npingOutput="$(sudo ip netns exec ns1 nping --udp -c 1 -p 50000 -g 50000 10.0.0.2)"
if [[ $npingOutput != *"Rcvd: 1"* ]]; then
  echo "Test failed (2)"
  exit 1
fi

set +e
echo "(3) Trying to ping (ns1->host) (not allowed)"
sudo ip netns exec ns1 ping -c 2 -i 0.5 10.0.0.2
if [[ $? == 0 ]]; then
  echo "Test failed (3)"
  exit 1
fi

echo "(4) Trying to ping (host->ns1) (not allowed)"
sudo ping -c 2 -i 0.5 10.0.0.1
if [[ $? == 0 ]]; then
  echo "Test failed (4)"
  exit 1
fi

echo "Test PASSED"
exit 0
