/*
 * firewall API
 *
 * Firewall Service
 *
 * API version: 2.0
 * Generated by: Swagger Codegen (https://github.com/swagger-api/swagger-codegen.git)
 */

package swagger

type SessionTable struct {

	// Source IP
	Src string `json:"src,omitempty"`

	// Destination IP
	Dst string `json:"dst,omitempty"`

	// Level 4 Protocol.
	L4proto string `json:"l4proto,omitempty"`

	// Source Port
	Sport int32 `json:"sport,omitempty"`

	// Destination
	Dport int32 `json:"dport,omitempty"`

	// Connection state.
	State string `json:"state,omitempty"`

	// Last packet matching the connection
	Eta int32 `json:"eta,omitempty"`
}