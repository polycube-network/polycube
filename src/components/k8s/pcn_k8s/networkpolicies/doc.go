// Package networkpolicies contains logic for parsing policies.
// It parses policies from kubernetes and polycube and gets back
// rules in a format that the firewall can understand.
// Then, the network policy manager, will take care of managing
// firewall managers inside its node
package networkpolicies
