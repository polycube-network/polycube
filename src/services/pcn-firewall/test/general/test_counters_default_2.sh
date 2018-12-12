# PING for testing default rule counter

source "${BASH_SOURCE%/*}/../helpers.bash"

function fwcleanup {
  set +e
  polycubectl firewall del fw
  delete_veth 2
}
trap fwcleanup EXIT

echo -e '\nTest counters \n'
set -e

create_veth 2

polycubectl firewall add fw
polycubectl firewall fw set loglevel=TRACE
polycubectl firewall fw ports add fw-p1
polycubectl firewall fw ports add fw-p2
polycubectl firewall fw ports fw-p1 set peer=veth1
polycubectl firewall fw ports fw-p2 set peer=veth2
polycubectl firewall fw chain INGRESS set default=FORWARD
polycubectl firewall fw chain EGRESS set default=FORWARD
set +e

#ping
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1

OUTPUT=$(polycubectl firewall fw chain INGRESS stats show -yaml)
if [[ $OUTPUT != *"pkts: 2"*  ]]; then
	echo "Failed.";
	exit 1;
fi

if [[ $OUTPUT != *"bytes: 196"*  ]]; then
	echo "Failed.";
	exit 1;
fi

echo "PASSED"

exit 0


