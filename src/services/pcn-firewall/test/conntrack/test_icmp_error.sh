source "${BASH_SOURCE%/*}/helpers.bash"

function fwcleanup {
  set +e
  polycubectl firewall del fw
  delete_veth 2
}
trap fwcleanup EXIT

set -e
set -x

create_veth 2

polycubectl firewall add fw
polycubectl firewall fw set loglevel=DEBUG
polycubectl firewall fw ports add fw-p1
polycubectl firewall fw ports add fw-p2
polycubectl firewall fw ports fw-p1 set peer=veth1
polycubectl firewall fw ports fw-p2 set peer=veth2

# Allowing only ICMP errors to be freely forwarded
polycubectl firewall fw chain INGRESS append l4proto=UDP action=FORWARD
polycubectl firewall fw chain INGRESS append l4proto=ICMP conntrack=RELATED action=FORWARD
polycubectl firewall fw chain INGRESS append l4proto=ICMP action=DROP

polycubectl firewall fw chain EGRESS append l4proto=UDP action=FORWARD
polycubectl firewall fw chain EGRESS append l4proto=ICMP conntrack=RELATED action=FORWARD
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
echo "(3) Trying to ping (ns1->ns2) (not allowed)"
sudo ip netns exec ns1 ping -c 2 -i 0.5 10.0.0.2
if [[ $? == 0 ]]; then
  echo "Test failed (3)"
  exit 1
fi

echo "(4) Trying to ping (ns2->ns1) (not allowed)"
sudo ip netns exec ns2 ping -c 2 -i 0.5 10.0.0.1
if [[ $? == 0 ]]; then
  echo "Test failed (4)"
  exit 1
fi

echo "Test PASSED"
exit 0
