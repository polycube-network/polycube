# PING for testing counters

source "${BASH_SOURCE%/*}/../helpers.bash"

function fwsetup {
  polycubectl firewall add fw
  polycubectl attach fw veth1
  polycubectl firewall fw chain INGRESS set default=DROP
  polycubectl firewall fw chain EGRESS set default=DROP
}

function fwcleanup {
  set +e
  #polycubectl detach fw veth1
  polycubectl firewall del fw
  delete_veth 2
}
trap fwcleanup EXIT

echo -e '\nTest counters \n'
set -e

create_veth 2

fwsetup

#matched rules
polycubectl firewall fw chain INGRESS rule add 0 src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=ACCEPT

#EGRESS CHAIN
polycubectl firewall fw chain EGRESS rule add 0 src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=ACCEPT

#ping
sudo ip netns exec ns1 ping 10.0.0.2 -c 2 -i 0.5 -w 1
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

