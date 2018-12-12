
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

pcn-iptables -P INPUT ACCEPT
pcn-iptables -P OUTPUT ACCEPT

ping $ip -W 1 -c 2 -W 2


pcn-iptables -P INPUT DROP

test_fail ping $ip -W 1 -c 2 -W 2

pcn-iptables -A INPUT -s $ip -d 10.0.22.0/24 -p tcp --sport 80 --dport 90 --tcp-flags SYN SYN -j ACCEPT

test_fail ping $ip -W 1 -c 2 -W 2

pcn-iptables -A INPUT -s $ip -d $ip -p tcp --sport 80 --dport 90 --tcp-flags SYN SYN -j ACCEPT

test_fail ping $ip -W 1 -c 2 -W 2

pcn-iptables -A INPUT -s $ip -p icmp -j ACCEPT

ping $ip -W 1 -c 2 -W 2


pcn-iptables -P OUTPUT DROP

test_fail ping $ip -W 1 -c 2 -W 2

pcn-iptables -A OUTPUT -d $ip -s 10.0.22.0/24 -p tcp --sport 80 --dport 90 --tcp-flags SYN SYN -j ACCEPT

test_fail ping $ip -W 1 -c 2 -W 2

pcn-iptables -A OUTPUT -s $ip -d $ip -p tcp --sport 80 --dport 90 --tcp-flags SYN SYN -j ACCEPT

test_fail ping $ip -W 1 -c 2 -W 2


pcn-iptables -A OUTPUT -d $ip -p icmp  -j ACCEPT

ping $ip -W 1 -c 2 -W 2

