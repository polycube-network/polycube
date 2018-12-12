
source "${BASH_SOURCE%/*}/helpers.bash"

#Test UDP

function iptablescleanup {
    set +e
    polycubectl iptables del pcn-iptables
    delete_veth 2
    sudo pkill -SIGTERM netcat
}
trap iptablescleanup EXIT

echo -e "\nTest $0 \n"
set -e
set -x

create_veth_net 2

enable_ip_forwarding

polycubectl iptables add pcn-iptables loglevel=TRACE

#pcn-iptables -P INPUT DROP
#pcn-iptables -P OUTPUT DROP
pcn-iptables -P FORWARD DROP

# Allow connections to be started only from ns2->ns1
pcn-iptables -A FORWARD -s 10.0.1.1 -p udp -m conntrack --ctstate=ESTABLISHED -j ACCEPT
pcn-iptables -A FORWARD -s 10.0.1.1 -p icmp -j ACCEPT


pcn-iptables -A FORWARD -s 10.0.2.1 -p udp -m conntrack --ctstate=NEW -j ACCEPT
pcn-iptables -A FORWARD -s 10.0.2.1 -p udp -m conntrack --ctstate=ESTABLISHED -j ACCEPT
pcn-iptables -A FORWARD -s 10.0.2.1 -p icmp -j ACCEPT

echo "UDP Conntrack Test"

echo "(1) Sending NOT allowed NEW UDP packet"
npingOutput="$(sudo ip netns exec ns1 nping --udp -c 1 -p 50002 -g 50001 10.0.2.1)"
polycubectl pcn-iptables session-table show

if [[ $npingOutput == *"Rcvd: 1"* ]]; then
  echo "Test failed (1)"
  exit 1
fi


echo "(2) Sending allowed UDP packet"
npingOutput="$(sudo ip netns exec ns2 nping --udp -c 1 -p 50001 --source-port 50002 10.0.1.1)"
polycubectl pcn-iptables session-table show
if [[ $npingOutput != *"Rcvd: 1"* ]]; then
  echo "Test failed (2)"
  exit 1
fi

echo "(3) Sending allowed ESTABLISHED UDP packet"
npingOutput="$(sudo ip netns exec ns1 nping --udp -c 1 -p 50002 -g 50001 10.0.2.1)"
polycubectl pcn-iptables session-table show

if [[ $npingOutput != *"Rcvd: 1"* ]]; then
  echo "Test failed (3)"
  exit 1
fi

sleep 5

echo "(4) Sending another allowed ESTABLISHED UDP packet"
npingOutput="$(sudo ip netns exec ns1 nping --udp -c 1 -p 50002 -g 50001 10.0.2.1)"
polycubectl pcn-iptables session-table show

if [[ $npingOutput != *"Rcvd: 1"* ]]; then
  echo "Test failed (4)"
  exit 1
fi

echo "Test PASSED"
exit 0
