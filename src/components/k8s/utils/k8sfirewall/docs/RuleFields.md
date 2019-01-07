# RuleFields

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**Src** | **string** | Source IP Address. | [optional] [default to null]
**Dst** | **string** | Destination IP Address. | [optional] [default to null]
**L4proto** | **string** | Level 4 Protocol. | [optional] [default to null]
**Sport** | **int32** | Source L4 Port | [optional] [default to null]
**Dport** | **int32** | Destination L4 Port | [optional] [default to null]
**Tcpflags** | **string** | TCP flags. Allowed values: SYN, FIN, ACK, RST, PSH, URG, CWR, ECE. ! means set to 0. | [optional] [default to null]
**Conntrack** | **string** | Connection status (NEW, ESTABLISHED, RELATED, INVALID) | [optional] [default to null]
**Action** | **string** | Action if the rule matches. Default is DROP. | [optional] [default to null]
**Description** | **string** | Description of the rule. | [optional] [default to null]

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


