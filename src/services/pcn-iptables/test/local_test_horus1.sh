
source "${BASH_SOURCE%/*}/helpers.bash"

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

pcn-iptables -P INPUT ACCEPT

polycubectl pcn-iptables set horus=ON

ping $ip -W 1 -c 2 -W 2

pcn-iptables -A INPUT -s $ip -j DROP

test_fail ping $ip -W 1 -c 2 -W 2

pcn-iptables -D INPUT -s $ip -j DROP
pcn-iptables -S INPUT
pcn-iptables -L INPUT

ping $ip -W 1 -c 2 -W 2

# check if rules are still working after horus disabled again

polycubectl pcn-iptables set horus=OFF

ping $ip -W 1 -c 2 -W 2

pcn-iptables -A INPUT -s $ip -j DROP

test_fail ping $ip -W 1 -c 2 -W 2

pcn-iptables -D INPUT -s $ip -j DROP

ping $ip -W 1 -c 2 -W 2
