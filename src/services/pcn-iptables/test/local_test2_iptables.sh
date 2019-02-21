
source "${BASH_SOURCE%/*}/helpers.bash"

# Test rule Insert, Append, Delete

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

bpf-iptables -A INPUT -s $ip -j ACCEPT

ping $ip -W 1 -c 2 -W 2

bpf-iptables -I INPUT -s $ip -j DROP

test_fail ping $ip -W 1 -c 2 -W 2

bpf-iptables -D INPUT -s $ip -j DROP

ping $ip -W 1 -c 2 -W 2
