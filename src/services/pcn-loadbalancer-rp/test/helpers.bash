function create_veth {
    # Create new namespace
    sudo ip netns add ns${i}
    # Add new veth interface
    sudo ip link add veth${i} type veth peer name veth${i}_ netns ns${i}
    # Enable veth on both root and newly created namespace
    sudo ip netns exec ns${i} ip link set dev veth${i}_ up
    sudo ip link set dev veth${i} up
}

function test_weight {
  # Test service and service backends creation and deletion. Test also different backend weights
  polycubectl lbrp lb0 service add 10.0.0.1 80 TCP name=my-service
  polycubectl lbrp lb0 service 10.0.0.1 80 TCP backend add 192.178.1.11 name=backend1 port=80 weight=20
  polycubectl lbrp lb0 service 10.0.0.1 80 TCP backend add 192.178.1.12 name=backend2 port=80 weight=30
  polycubectl lbrp lb0 service 10.0.0.1 80 TCP backend add 192.178.1.13 name=backend3 port=80 weight=20
  polycubectl lbrp lb0 service 10.0.0.1 80 TCP backend show
  polycubectl lbrp lb0 service 10.0.0.1 80 TCP show
  polycubectl lbrp lb0 show
  polycubectl lbrp lb0 service 10.0.0.1 80 TCP backend del 192.178.1.11
  polycubectl lbrp lb0 service 10.0.0.1 80 TCP backend del 192.178.1.13
  polycubectl lbrp lb0 service 10.0.0.1 80 TCP backend del 192.178.1.12
  polycubectl lbrp lb0 service 10.0.0.1 80 TCP backend show
  polycubectl lbrp lb0 service 10.0.0.1 80 TCP show
  polycubectl lbrp lb0 service del 10.0.0.1 80 TCP
  polycubectl lbrp lb0 show
}

function test_service_reachability {
  polycubectl lb0 service add 10.0.0.1 80 ALL name=my-service
  polycubectl lb0 service 10.0.0.1 80 tcp backend add 192.178.1.2 port=80 weight=20 name=backend-tcp
  polycubectl lb0 service 10.0.0.1 80 udp backend add 192.178.1.2 port=80 weight=20 name=backend-udp
  polycubectl lb0 service 10.0.0.1 0 icmp backend add 192.178.1.2 port=0 weight=20 name=backend-icmp

  sudo ip netns exec ns1 ping 10.0.0.1 -c 2 -w 2

  polycubectl lbrp lb0 service del 10.0.0.1 80 TCP

  polycubectl lbrp lb0 show
}