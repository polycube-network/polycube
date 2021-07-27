# Monitoring examples

This folder contains a set of monitoring dataplanes, which can be used as examples for the Dynamic Monitoring service.

- [packet_counter.json](packet_counter.json):
  - the `packets_total` metric represents the number of packets that have traversed the attached network interface (INGRESS)

- [ntp_packets_counter.json](ntp_packets_counter.json):
  - the `ntp_packets_total` metric represents the number of NTP packets that have traversed the attached network interface (INGRESS) 

- [ntp_packets_ntp_mode_private_counters.json](ntp_packets_ntp_mode_private_counters.json):
  - the `ntp_packets_total` metric represents the number of NTP packets that have traversed the attached network interface (INGRESS);
  - the `ntp_mode_private_packets_total` metric represents the number of NTP packets with NTP_MODE = 7 (MODE_PRIVATE) that have traversed the attached network interface (INGRESS)

All counters are *incremental*, hence their values are monotonically increasing.


Unfortunately the dataplane code (eBPF restricted C) contained in the above JSONs is not easy to read for a human, due to the formatting limitations of the JSON format. A more human friendly version can be produced by unescaping the code by using [this online free tool](https://www.freeformatter.com/json-escape.html) or our [Formatter](../tools/formatter.py) script.

The same tool can be used also to escape any multiline code strings in order to create new valid injectable dataplanes.
