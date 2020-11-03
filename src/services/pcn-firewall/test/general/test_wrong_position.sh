# PING testing rule appending

source "${BASH_SOURCE%/*}/../helpers.bash"

function fwsetup {
  polycubectl firewall add fw loglevel=DEBUG
  polycubectl attach fw veth1
  polycubectl firewall fw chain INGRESS set default=DROP
  polycubectl firewall fw chain EGRESS set default=DROP
}

function fwcleanup {
  set +e
  polycubectl firewall del fw
  delete_veth 2
}
trap fwcleanup EXIT

echo -e '\nTest wrong position \n'
set -e
set -x

create_veth 2

fwsetup

# INGRESS one rule
polycubectl firewall fw chain INGRESS append src=10.0.0.1 dst=10.0.0.2 l4proto=ICMP action=ACCEPT

#EGRESS multiple rules
polycubectl firewall fw chain EGRESS append src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=ACCEPT
polycubectl firewall fw chain EGRESS append src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=ACCEPT
polycubectl firewall fw chain EGRESS append src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=ACCEPT
polycubectl firewall fw chain EGRESS append src=10.0.0.2/32 dst=10.0.0.1/32 l4proto=ICMP action=ACCEPT

# Test out of bound access
set +e
polycubectl fw chain EGRESS rule del -1
polycubectl fw chain EGRESS rule del 4
polycubectl fw chain EGRESS rule del 5
polycubectl fw chain EGRESS rule del 10
set -e

# test fw to be still alive
polycubectl fw chain EGRESS show

set +e
polycubectl fw chain EGRESS rule show -1
polycubectl fw chain EGRESS rule show 4
polycubectl fw chain EGRESS rule show 5
polycubectl fw chain EGRESS rule show 10
set -e

# test fw to be still alive
polycubectl fw chain EGRESS show

set +e
polycubectl fw chain INGRESS rule del -1
polycubectl fw chain INGRESS rule del 1
polycubectl fw chain INGRESS rule del 2
polycubectl fw chain INGRESS rule del 10

polycubectl fw chain INGRESS rule show -1
polycubectl fw chain INGRESS rule show 1
polycubectl fw chain INGRESS rule show 2
polycubectl fw chain INGRESS rule show 10
set -e

# test fw to be still alive
polycubectl fw chain EGRESS show

polycubectl fw chain EGRESS show rule 0
polycubectl fw chain EGRESS show rule 1
polycubectl fw chain EGRESS show rule 2
polycubectl fw chain EGRESS show rule 3

polycubectl fw chain INGRESS show rule 0



