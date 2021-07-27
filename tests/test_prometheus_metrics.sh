#!/bin/bash

source "${BASH_SOURCE%/*}/helpers.bash"

# the expected result
expected_results=('ddos_stats_pkts_packets{cubeName="d1"} 0.000000'
'ddos_stats_pkts_packets{cubeName="d1"} 0.000000'
'ddos_blacklist_src_addresses{cubeName="d1"} 4.000000'
'ddos_blacklist_dst_addresses{cubeName="d1"} 0.000000'
'router_ip_ports{cubeName="r1"} 2.000000'
'router_secondaryip_ports{cubeName="r1"} 3.000000'
'router_ports{cubeName="r1"} 2.000000'
'router_route_routes{cubeName="r1"} 2.000000'
'router_arp_table_entries{cubeName="r1"} 0.000000')

set -e
set -x

function cleanup {
  set +e
  polycubectl del d1
  polycubectl del r1
  echo "FAIL"
}

trap cleanup EXIT

polycubectl ddosmitigator add d1

polycubectl d1 blacklist-src add 10.0.0.1

polycubectl d1 blacklist-src add 10.1.1.2

polycubectl d1 blacklist-src add 10.1.1.3

polycubectl d1 blacklist-src add 10.1.1.4

polycubectl router add r1

polycubectl r1 ports add p1

polycubectl r1 ports p1 set ip=10.1.1.1/24

polycubectl r1 ports add p2

polycubectl r1 ports p2 set ip=10.1.2.1/24

polycubectl r1 ports p1 secondaryip add 10.1.1.10/24

polycubectl r1 ports p1 secondaryip add 10.1.1.11/24

polycubectl r1 ports p1 secondaryip add 10.1.1.12/24

curl_result=$(curl -s localhost:9000/polycube/v1/metrics)

ok=0

for i in "${expected_results[@]}"
do
  if [[ "$curl_result" == *"$i"* ]]
  then
    ((ok=ok+1))
  fi
done

diff -q <(echo "$ok") <(echo "${#expected_results[@]}")

polycubectl del d1
polycubectl del r1


set +x
trap - EXIT
echo "SUCCESS"







