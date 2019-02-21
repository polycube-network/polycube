
source "${BASH_SOURCE%/*}/helpers.bash"

#Test conntrack starting connection, and conntrack table

function iptablescleanup {
    set +e
    bpf-iptables-clean
    delete_veth 2
    sudo pkill -SIGTERM netcat
}
trap iptablescleanup EXIT

function test_tcp {
    sudo ip netns exec ns2 netcat -l -w 5 $1&
    sleep 2
    sudo ip netns exec ns1 netcat -w 2 -nvz 10.0.2.1 $1
    sleep 4
}

function test_tcp_fail {
    sudo ip netns exec ns2 netcat -l -w 5 $1&
    sleep 2
    test_fail sudo ip netns exec ns1 netcat -w 2 -nvz 10.0.2.1 $1
    sleep 4
}

echo -e "\nTest $0 \n"
set -e
set -x

create_veth_net 2

enable_ip_forwarding

polycubectl iptables add bpf-iptables loglevel=TRACE

#bpf-iptables -P INPUT DROP
#bpf-iptables -P OUTPUT DROP
bpf-iptables -P FORWARD DROP

test_tcp_fail "9090"

# Allow connections to be started only from ns1->ns2
bpf-iptables -A FORWARD -s 10.0.2.1 -p tcp -m conntrack --ctstate=ESTABLISHED -j ACCEPT
bpf-iptables -A FORWARD -s 10.0.2.1 -p tcp -m conntrack --ctstate=INVALID -j DROP

bpf-iptables -A FORWARD -s 10.0.1.1 -p tcp -m conntrack --ctstate=NEW -j ACCEPT
bpf-iptables -A FORWARD -s 10.0.1.1 -p tcp -m conntrack --ctstate=ESTABLISHED -j ACCEPT
bpf-iptables -A FORWARD -s 10.0.1.1 -p tcp -m conntrack --ctstate=INVALID -j DROP

test_tcp "9091"

#Flush policies
bpf-iptables -F FORWARD

# Allow connections to be started only from ns2->ns1
bpf-iptables -A FORWARD -s 10.0.1.1 -p tcp -m conntrack --ctstate=ESTABLISHED -j ACCEPT
bpf-iptables -A FORWARD -s 10.0.1.1 -p tcp -m conntrack --ctstate=INVALID -j DROP

bpf-iptables -A FORWARD -s 10.0.2.1 -p tcp -m conntrack --ctstate=NEW -j ACCEPT
bpf-iptables -A FORWARD -s 10.0.2.1 -p tcp -m conntrack --ctstate=ESTABLISHED -j ACCEPT
bpf-iptables -A FORWARD -s 10.0.2.1 -p tcp -m conntrack --ctstate=INVALID -j DROP

test_tcp_fail "9092"
