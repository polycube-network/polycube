
source "${BASH_SOURCE%/*}/helpers.bash"

#Test conntrack in forward chain

function iptablescleanup {
    set +e
    polycubectl iptables del pcn-iptables
    sudo ip netns del ns1
    sudo ip link del veth1
    sudo ip netns del ns2
    sudo ip link del veth2
    sudo pkill -SIGTERM netcat
}
trap iptablescleanup EXIT

function test_tcp {
    sudo ip netns exec ns2 netcat -l -w 5 60123&
    sleep 2
    sudo ip netns exec ns1 netcat -w 5 -nvz 10.0.2.1 60123
    sleep 4
}

function test_tcp_fail {
    sudo ip netns exec ns2 netcat -l -w 5 60123&
    sleep 2
    test_fail sudo ip netns exec ns1 netcat -w 5 -nvz 10.0.2.1 60123
    sleep 4
}

echo -e "\nTest $0 \n"
set -e
set -x


launch_iptables

enable_ip_forwarding

#create ns
for i in `seq 1 2`;
do
    sudo ip netns add ns${i}
    sudo ip link add veth${i}_ type veth peer name veth${i}
    sudo ip link set veth${i}_ netns ns${i}
    sudo ip netns exec ns${i} ip link set dev veth${i}_ up
    sudo ip link set dev veth${i} up

    sudo ifconfig veth${i} 10.0.${i}.254/24 up

    sudo ip netns exec ns${i} ifconfig veth${i}_ 10.0.${i}.1/24
    sudo ip netns exec ns${i} sudo ip route add default via 10.0.${i}.254

done

sudo ip netns exec ns1 ping 10.0.2.1 -c 2 -W 2

pcn-iptables -P INPUT DROP
pcn-iptables -P OUTPUT DROP

sudo ip netns exec ns1 ping 10.0.2.1 -c 2 -W 2

pcn-iptables -P INPUT ACCEPT
pcn-iptables -P OUTPUT ACCEPT
pcn-iptables -P FORWARD DROP

test_fail sudo ip netns exec ns1 ping 10.0.2.1 -c 2 -W 2

pcn-iptables -P INPUT DROP
pcn-iptables -P OUTPUT DROP

pcn-iptables -I FORWARD -s 10.0.0.0/8 -j ACCEPT

sudo ip netns exec ns1 ping 10.0.2.1 -c 2 -W 2

pcn-iptables -D FORWARD -s 10.0.0.0/8 -j ACCEPT

test_fail sudo ip netns exec ns1 ping 10.0.2.1 -c 2 -W 2

pcn-iptables -A FORWARD -s 10.0.0.0/8 -j ACCEPT

sudo ip netns exec ns1 ping 10.0.2.1 -c 2 -W 2

pcn-iptables -D FORWARD -s 10.0.0.0/8 -j ACCEPT

test_fail sudo ip netns exec ns1 ping 10.0.2.1 -c 2 -W 2

pcn-iptables -I FORWARD 1 -s 10.0.0.0/8 -j ACCEPT

sudo ip netns exec ns1 ping 10.0.2.1 -c 2 -W 2

pcn-iptables -D FORWARD -s 10.0.0.0/8 -j ACCEPT

test_fail sudo ip netns exec ns1 ping 10.0.2.1 -c 2 -W 2

