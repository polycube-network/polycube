# PING for testing counters after new rule is inserted

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
polycubectl firewall fw set loglevel=DEBUG
polycubectl firewall fw ports add fw-p1
polycubectl firewall fw ports add fw-p2
polycubectl firewall fw ports fw-p1 set peer=veth1
polycubectl firewall fw ports fw-p2 set peer=veth2

# Matched rules
polycubectl firewall fw chain INGRESS rule add 0 src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=FORWARD
polycubectl firewall fw chain EGRESS rule add 0 src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=FORWARD

#ping
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1

# New stub rules
polycubectl firewall fw chain INGRESS rule add 1 dst=10.0.0.2 l4proto=TCP sport=1000 action=FORWARD
polycubectl firewall fw chain EGRESS append src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=UDP dport=1000 action=FORWARD

OUTPUT=$(polycubectl firewall fw chain INGRESS stats 0 show pkts)
if [ $OUTPUT != 2 ]; then
	echo $OUTPUT
	echo "Failed 1.";
	exit 1;
fi

OUTPUT=$(polycubectl firewall fw chain EGRESS stats 0 show pkts)
if [ $OUTPUT != 2 ]; then
	echo $OUTPUT
	echo "Failed 2.";
	exit 1;
fi

OUTPUT=$(polycubectl firewall fw chain INGRESS stats 0 show bytes)
if [ $OUTPUT != 196 ]; then
	echo $OUTPUT
	echo "Failed 3.";
	exit 2;
fi

OUTPUT=$(polycubectl firewall fw chain EGRESS stats 0 show bytes)
if [ $OUTPUT != 196 ]; then
	echo $OUTPUT
	echo "Failed 4.";
	exit 2;
fi

echo "Success."

