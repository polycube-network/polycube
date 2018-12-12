
source "${BASH_SOURCE%/*}/helpers.bash"

# This test is going to stress INPUT and OUTPUT chains and relative default policies
# By contacting, via ping, $ip address

function iptablescleanup {
    set +e
    polycubectl iptables del pcn-iptables
}
trap iptablescleanup EXIT

echo -e "\nTest $0 \n"
set -e
set -x

launch_iptables

ping $ip -W 1 -c 2 -W 2

polycubectl pcn-iptables chain INPUT set default=ACCEPT
polycubectl pcn-iptables chain OUTPUT set default=ACCEPT

ping $ip -W 1 -c 2 -W 2

polycubectl pcn-iptables chain INPUT set default=DROP

test_fail ping $ip -W 1 -c 2 -W 2

polycubectl pcn-iptables chain INPUT rule add 0 src=$ip dst=10.0.22.0/24 l4proto=TCP sport=80 dport=90 tcpflags='SYN' action=ACCEPT

test_fail ping $ip -W 1 -c 2 -W 2

polycubectl pcn-iptables chain INPUT rule add 1 src=$ip dst=$ip l4proto=TCP sport=80 dport=90 tcpflags='SYN' action=ACCEPT

test_fail ping $ip -W 1 -c 2 -W 2

polycubectl pcn-iptables chain INPUT rule add 2 src=$ip l4proto=ICMP action=ACCEPT

ping $ip -W 1 -c 2 -W 2



polycubectl pcn-iptables chain OUTPUT set default=DROP

test_fail ping $ip -W 1 -c 2 -W 2

polycubectl pcn-iptables chain OUTPUT rule add 0 dst=$ip src=10.0.22.0/24 l4proto=TCP sport=80 dport=90 tcpflags='SYN' action=ACCEPT

test_fail ping $ip -W 1 -c 2 -W 2

polycubectl pcn-iptables chain OUTPUT rule add 1 src=$ip dst=$ip l4proto=TCP sport=80 dport=90 tcpflags='SYN' action=ACCEPT

test_fail ping $ip -W 1 -c 2 -W 2

polycubectl pcn-iptables chain OUTPUT rule add 2 dst=$ip l4proto=ICMP action=ACCEPT

ping $ip -W 1 -c 2 -W 2

