package networkpolicies

type PolicyParser interface {
	DeployPolicy()
	UpdatePolicy()
	RemovePolicy()
}
