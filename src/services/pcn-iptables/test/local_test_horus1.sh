
source "${BASH_SOURCE%/*}/helpers.bash"

function iptablescleanup {
    set +e
    bpf-iptables-clean
}
trap iptablescleanup EXIT

echo -e "\nTest $0 \n"
set -e
set -x

launch_iptables

ping $ip -W 1 -c 2 -W 2

bpf-iptables -P INPUT ACCEPT
bpf-iptables -P OUTPUT ACCEPT

ping $ip -W 1 -c 2 -W 2

bpf-iptables -P INPUT DROP

test_fail ping $ip -W 1 -c 2 -W 2

bpf-iptables -P INPUT ACCEPT

polycubectl bpf-iptables set horus=ON

ping $ip -W 1 -c 2 -W 2

bpf-iptables -A INPUT -s $ip -j DROP

test_fail ping $ip -W 1 -c 2 -W 2

bpf-iptables -D INPUT -s $ip -j DROP
bpf-iptables -S INPUT
bpf-iptables -L INPUT

ping $ip -W 1 -c 2 -W 2

# check if rules are still working after horus disabled again

polycubectl bpf-iptables set horus=OFF

ping $ip -W 1 -c 2 -W 2

bpf-iptables -A INPUT -s $ip -j DROP

test_fail ping $ip -W 1 -c 2 -W 2

bpf-iptables -D INPUT -s $ip -j DROP

ping $ip -W 1 -c 2 -W 2
