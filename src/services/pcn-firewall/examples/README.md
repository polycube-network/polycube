# Firewall Examples
The example folder contains a set of simple scripts to understand how the firewall service and cli works.

## Prerequisites
All scripts assume that polycubed has been already launched, and that there is a standard cube running with two ports belonging to two namespaces already created and configured. Moreover, a firewall instance should be running and attached to one of the standard cube's port.
To set up all the needed components, please execute the script [setup_env.sh](./setup_env.sh).
Moreover, these examples contain a set of rules to allow some traffic, while denying the rest of it. To make this happen, since the default policy is ACCEPT, the setup script automatically patches the default rule to DROP.

## Examples:
- [Ping](./allow_ping.sh): Connects the firewall to one of the standard cube's port, and allows only the ICMP echo requests/responses. In order to test that the configuration succeeded, you can launch the script [test_ping.sh](./test_ping.sh).
- [TCP](./allow_tcp.sh): Connects the firewall to one of the standard cube's port, and allows only the TCP traffic. In order to test that the configuration succeeded, you can launch the script [test_tcp.sh](./test_tcp.sh). If the test_tcp script fails, please install the nping program.
- [Advanced TCP](./allow_tcp_adv.sh): Connects the firewall to one of the standard cube's port, and allows only specific TCP traffic, specifying ports and flags. In order to test that the configuration succeeded, you can launch the script [test_tcp_adv.sh](./test_tcp_adv.sh). If the test_tcp script fails, please install the nping program.
- [Append](./use_append.sh): This example is like the Ping one, as the rule set is the same, but it gives an example on how to insert rules at the end of the chain without specifying their ID. At the end of the script, there is already a ping command to test the configuration.
- [Counters](./use_counters.sh): This example is like the Ping one, as the rule set is the same, but it shows how to query and flush the counters. After a ping, that requires two packets matching the rule 0 to be traverse each chain, it executes three different queries to get the statistics. After all queries have been completed, it reset the counters flushing them back to 0.
- [Batch](use_batch.sh): This example is like the Ping one, as the rule set is the same, but it shows how to use batch rule injection. This mode is strongly suggested when more than one rule has to be inserted, like in the example. **For each chain**, after the rules have been inserted, the rule set is automatically applied at once, requiring a single interaction with the datapath.
- [Host Mode](./host_mode.sh): This example shows how to use the firewall in the host mode, intercepting the traffic **from the outside to the host**. At the moment it is not possible to intercept traffic in the other direction. This example considers the physical interface connected to the internet.

Please note that some example does not voluntarily delete used resources like firewall or network namespace, since a user can play with multiple rules (e.g. allow IP and TCP). Thus, the behaviour of some tests may change depending on the allowed scripts run.

To cleanup the entire environment or only the firewall's rules, refer to the following sections.

## Reset
To reset the firewall's rules, please use the script [reset_firewall.sh](./reset_firewall.sh).

## Cleanup
 To cleanup the environment, please use the script [cleanup_env.sh](./cleanup_env.sh).