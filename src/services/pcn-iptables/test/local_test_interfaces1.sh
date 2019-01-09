
source "${BASH_SOURCE%/*}/helpers.bash"

#test forwarding chain between namespaces
#test filters on input interfaces (-i)

function iptablescleanup {
    set +e
    polycubectl iptables del pcn-iptables
    sudo ip netns del ns1
    sudo ip link del veth1
    sudo ip netns del ns2
    sudo ip link del veth2
}
trap iptablescleanup EXIT

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

# test -i on FORWARD chain

pcn-iptables -P FORWARD DROP

test_fail sudo ip netns exec ns1 ping 10.0.2.1 -c 2 -W 2

pcn-iptables -A FORWARD -i veth1 -j ACCEPT

test_fail sudo ip netns exec ns1 ping 10.0.2.1 -c 2 -W 2

pcn-iptables -A FORWARD -i veth2 -j ACCEPT

sudo ip netns exec ns1 ping 10.0.2.1 -c 2 -W 2

pcn-iptables -D FORWARD -i veth1 -j ACCEPT

test_fail sudo ip netns exec ns1 ping 10.0.2.1 -c 2 -W 2

pcn-iptables -D FORWARD -i veth2 -j ACCEPT

test_fail sudo ip netns exec ns1 ping 10.0.2.1 -c 2 -W 2

pcn-iptables -P FORWARD ACCEPT

sudo ip netns exec ns1 ping 10.0.2.1 -c 2 -W 2

# test -i -o on INPUT/ OUTPUT chains

# obtain default route interface, e.g. ens3
iface=$(sudo ip route | grep default | awk '{print $5}')

pcn-iptables -F FORWARD

sudo ip netns exec ns1 ping 10.0.2.1 -c 2 -W 2

pcn-iptables -P FORWARD DROP

test_fail sudo ip netns exec ns1 ping 10.0.2.1 -c 2 -W 2

pcn-iptables -P INPUT ACCEPT
pcn-iptables -P OUTPUT ACCEPT

ping $ip -c 2 -W 2

pcn-iptables -A INPUT -i $iface -j DROP

test_fail ping $ip -c 2 -W 2

pcn-iptables -D INPUT -i $iface -j DROP

ping $ip -c 2 -W 2

pcn-iptables -A OUTPUT -o $iface -j DROP

test_fail ping $ip -c 2 -W 2

pcn-iptables -D OUTPUT -o $iface -j DROP

ping $ip -c 2 -W 2
