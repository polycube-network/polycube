#!/bin/bash

source "${BASH_SOURCE%/*}/helpers.bash"

# the expected result
result='# HELP ddos_stats_pkts_packets Total Dropped Packets
# TYPE ddos_stats_pkts_packets counter
ddos_stats_pkts_packets{cubeName="d1"} 0.000000
# HELP k8switch_service_client_exported_services Number of services exported to the client
# TYPE k8switch_service_client_exported_services gauge
# HELP k8switch_backend_servers Number of backend servers that actually serve requests
# TYPE k8switch_backend_servers gauge
# HELP k8switch_fwd_table_entries Number of entries associated with the forwarding table
# TYPE k8switch_fwd_table_entries gauge
# HELP router_ip_ports Number of ports of the router with ip
# TYPE router_ip_ports gauge
router_ip_ports{cubeName="r1"} 2.000000
# HELP router_secondaryip_ports Number of secondary ip addresses of the router
# TYPE router_secondaryip_ports gauge
router_secondaryip_ports{cubeName="r1"} 3.000000
# HELP router_ports Number of ports on the router
# TYPE router_ports gauge
router_ports{cubeName="r1"} 2.000000
# HELP router_route_routes Number of entries in routing table
# TYPE router_route_routes gauge
router_route_routes{cubeName="r1"} 2.000000
# HELP router_arp_table_entries Number of entries in arp table
# TYPE router_arp_table_entries gauge
router_arp_table_entries{cubeName="r1"} 0.000000
# HELP simplebridge_entry_fdb_number_entries Number of entries associated with the filtering database
# TYPE simplebridge_entry_fdb_number_entries gauge
# HELP ddos_blacklist_src_addresses Number of addresses in blacklist-src
# TYPE ddos_blacklist_src_addresses gauge
ddos_blacklist_src_addresses{cubeName="d1"} 4.000000
# HELP ddos_blacklist_dst_addresses Number of addresses in blacklist-dst
# TYPE ddos_blacklist_dst_addresses gauge
ddos_blacklist_dst_addresses{cubeName="d1"} 0.000000
# HELP simpleforwarder_actions_entries Number of entries of the Actions table
# TYPE simpleforwarder_actions_entries gauge
# HELP pbforwarder_rules_number_rules Number of rules
# TYPE pbforwarder_rules_number_rules gauge'

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


diff -q <(echo "$result") <(echo "$(curl -s 'localhost:9000/polycube/v1/metrics')")

polycubectl del d1
polycubectl del r1

set +x
trap - EXIT
echo "SUCCESS"







