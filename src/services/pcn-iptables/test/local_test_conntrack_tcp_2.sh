
source "${BASH_SOURCE%/*}/helpers.bash"

#Test conntrack starting connection, and conntrack table

function iptablescleanup {
    set +e
    polycubectl iptables del pcn-iptables
    delete_veth 2
    sudo pkill -SIGTERM netcat
}
trap iptablescleanup EXIT

function test_tcp {
    sudo ip netns exec ns2 netcat -l -w 5 $1&
    sleep 2
    sudo ip netns exec ns1 netcat -w 5 -nvz 10.0.2.1 $1
    sleep 4
}

function test_tcp_fail {
    sudo ip netns exec ns2 netcat -l -w 5 $1&
    sleep 2
    test_fail sudo ip netns exec ns1 netcat -w 5 -nvz 10.0.2.1 $1
    sleep 4
}

echo -e "\nTest $0 \n"
set -e
set -x

create_veth_net 2

enable_ip_forwarding

polycubectl iptables add pcn-iptables loglevel=TRACE

#pcn-iptables -P INPUT DROP
#pcn-iptables -P OUTPUT DROP
pcn-iptables -P FORWARD DROP

test_tcp_fail "9090"

# Allow connections to be started only from ns1->ns2
pcn-iptables -A FORWARD -s 10.0.2.1 -p tcp -m conntrack --ctstate=ESTABLISHED -j ACCEPT
pcn-iptables -A FORWARD -s 10.0.2.1 -p tcp -m conntrack --ctstate=INVALID -j DROP

pcn-iptables -A FORWARD -s 10.0.1.1 -p tcp -m conntrack --ctstate=NEW -j ACCEPT
pcn-iptables -A FORWARD -s 10.0.1.1 -p tcp -m conntrack --ctstate=ESTABLISHED -j ACCEPT
pcn-iptables -A FORWARD -s 10.0.1.1 -p tcp -m conntrack --ctstate=INVALID -j DROP

test_tcp "9091"

#Flush policies
pcn-iptables -F FORWARD

# Allow connections to be started only from ns2->ns1
pcn-iptables -A FORWARD -s 10.0.1.1 -p tcp -m conntrack --ctstate=ESTABLISHED -j ACCEPT
pcn-iptables -A FORWARD -s 10.0.1.1 -p tcp -m conntrack --ctstate=INVALID -j DROP

pcn-iptables -A FORWARD -s 10.0.2.1 -p tcp -m conntrack --ctstate=NEW -j ACCEPT
pcn-iptables -A FORWARD -s 10.0.2.1 -p tcp -m conntrack --ctstate=ESTABLISHED -j ACCEPT
pcn-iptables -A FORWARD -s 10.0.2.1 -p tcp -m conntrack --ctstate=INVALID -j DROP

test_tcp_fail "9092"