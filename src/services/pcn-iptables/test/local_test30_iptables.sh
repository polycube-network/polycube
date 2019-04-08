
source "${BASH_SOURCE%/*}/helpers.bash"

# Test matchin rule counters, with rules > 64

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

function test_counters {
    count=$(pcn-iptables -L FORWARD | grep stats -A 3 | tail -n 1 | awk '{print $2}')
    #echo $count
    if [ "$count" -ne "5" ]; then
        echo "Expected count = 5"
        exit -1
    fi
}

function test_udp {
    sudo ip netns exec ns2 nping --udp 10.0.1.1
}

echo -e "\nTest $0 \n"
set -e
set -x

polycubectl iptables add pcn-iptables loglevel=TRACE

enable_ip_forwarding

IPTABLES="pcn-iptables"
CHAIN="FORWARD"

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

#test_udp

$IPTABLES -A $CHAIN -m conntrack --ctstate ESTABLISHED -j ACCEPT
$IPTABLES -A $CHAIN -m conntrack --ctstate NEW -s 10.0.2.1 -j ACCEPT

pcn-iptables -L FORWARD

test_udp

pcn-iptables -L FORWARD

test_counters

pcn-iptables -F FORWARD
polycubectl pcn-iptables set interactive=false

$IPTABLES -A $CHAIN -m conntrack --ctstate ESTABLISHED -j ACCEPT
$IPTABLES -A $CHAIN -m conntrack --ctstate NEW -s 10.0.2.1 -j ACCEPT
$IPTABLES -A $CHAIN -m conntrack --ctstate NEW -s 192.168.10.3 -j ACCEPT
$IPTABLES -A $CHAIN -m conntrack --ctstate NEW -s 192.168.10.4 -j ACCEPT
$IPTABLES -A $CHAIN -m conntrack --ctstate NEW -s 192.168.10.5 -j ACCEPT
$IPTABLES -A $CHAIN -m conntrack --ctstate NEW -s 192.168.10.6 -j ACCEPT
$IPTABLES -A $CHAIN -m conntrack --ctstate NEW -s 192.168.10.7 -j ACCEPT
$IPTABLES -A $CHAIN -m conntrack --ctstate NEW -s 192.168.10.8 -j ACCEPT
$IPTABLES -A $CHAIN -m conntrack --ctstate NEW -s 192.168.10.9 -j ACCEPT
$IPTABLES -A $CHAIN -m conntrack --ctstate NEW -s 192.168.10.10 -j ACCEPT
$IPTABLES -A $CHAIN -m conntrack --ctstate NEW -s 192.168.10.11 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.2 -p udp --dport 8080 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.2 -p udp --dport 8081 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.2 -p udp --dport 8082 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.2 -p udp --dport 8083 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.2 -p udp --dport 8084 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.2 -p udp --dport 8085 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.2 -p udp --dport 8086 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.2 -p udp --dport 8087 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.2 -p udp --dport 8088 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.3 -p udp --dport 8080 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.3 -p udp --dport 8081 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.3 -p udp --dport 8082 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.3 -p udp --dport 8083 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.3 -p udp --dport 8084 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.3 -p udp --dport 8085 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.3 -p udp --dport 8086 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.3 -p udp --dport 8087 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.3 -p udp --dport 8088 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.4 -p udp --dport 8080 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.4 -p udp --dport 8081 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.4 -p udp --dport 8082 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.4 -p udp --dport 8083 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.4 -p udp --dport 8084 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.4 -p udp --dport 8085 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.4 -p udp --dport 8086 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.4 -p udp --dport 8087 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.4 -p udp --dport 8088 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.5 -p udp --dport 8080 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.5 -p udp --dport 8081 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.5 -p udp --dport 8082 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.5 -p udp --dport 8083 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.5 -p udp --dport 8084 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.5 -p udp --dport 8085 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.5 -p udp --dport 8086 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.5 -p udp --dport 8087 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.5 -p udp --dport 8088 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.6 -p udp --dport 8080 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.6 -p udp --dport 8081 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.6 -p udp --dport 8082 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.6 -p udp --dport 8083 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.6 -p udp --dport 8084 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.6 -p udp --dport 8085 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.6 -p udp --dport 8086 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.6 -p udp --dport 8087 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.6 -p udp --dport 8088 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.7 -p udp --dport 8080 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.7 -p udp --dport 8081 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.7 -p udp --dport 8082 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.7 -p udp --dport 8083 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.7 -p udp --dport 8084 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.7 -p udp --dport 8085 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.7 -p udp --dport 8086 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.7 -p udp --dport 8087 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.7 -p udp --dport 8088 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.8 -p udp --dport 8080 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.8 -p udp --dport 8081 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.8 -p udp --dport 8082 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.8 -p udp --dport 8083 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.8 -p udp --dport 8084 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.8 -p udp --dport 8085 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.8 -p udp --dport 8086 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.8 -p udp --dport 8087 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.8 -p udp --dport 8088 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.9 -p udp --dport 8080 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.9 -p udp --dport 8081 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.9 -p udp --dport 8082 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.9 -p udp --dport 8083 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.9 -p udp --dport 8084 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.9 -p udp --dport 8085 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.9 -p udp --dport 8086 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.9 -p udp --dport 8087 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.9 -p udp --dport 8088 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.10 -p udp --dport 8080 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.10 -p udp --dport 8081 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.10 -p udp --dport 8082 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.10 -p udp --dport 8083 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.10 -p udp --dport 8084 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.10 -p udp --dport 8085 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.10 -p udp --dport 8086 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.10 -p udp --dport 8087 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.10 -p udp --dport 8088 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.11 -p udp --dport 8080 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.11 -p udp --dport 8081 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.11 -p udp --dport 8082 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.11 -p udp --dport 8083 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.11 -p udp --dport 8084 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.11 -p udp --dport 8085 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.11 -p udp --dport 8086 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.11 -p udp --dport 8087 -j ACCEPT
$IPTABLES -A $CHAIN -d 192.168.10.11 -p udp --dport 8088 -j ACCEPT

polycubectl pcn-iptables chain $CHAIN apply-rules

pcn-iptables -L FORWARD

test_udp

pcn-iptables -L FORWARD

test_counters
