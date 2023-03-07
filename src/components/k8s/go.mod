module github.com/polycube-network/polycube/src/components/k8s

go 1.12

require (
	github.com/aws/aws-sdk-go v1.20.15
	github.com/containernetworking/cni v0.7.1
	github.com/containernetworking/plugins v0.8.1
	github.com/coreos/etcd v3.3.13+incompatible
	github.com/coreos/go-iptables v0.4.1 // indirect
	github.com/imdario/mergo v0.3.7 // indirect
	github.com/safchain/ethtool v0.0.0-20190326074333-42ed695e3de8 // indirect
	github.com/segmentio/ksuid v1.0.2
	github.com/sirupsen/logrus v1.4.2
	github.com/stretchr/testify v1.4.0
	github.com/vishvananda/netlink v1.0.0
	github.com/vishvananda/netns v0.0.0-20190625233234-7109fa855b0f // indirect
	golang.org/x/net v0.0.0-20200707034311-ab3426394381
	golang.org/x/oauth2 v0.0.0-20191202225959-858c2ad4c8b6
	golang.org/x/sys v0.0.0-20200622214017-ed371f2e16b4
	k8s.io/api v0.20.0-alpha.2
	k8s.io/apimachinery v0.20.0-alpha.2
	k8s.io/client-go v0.20.0-alpha.2
)
