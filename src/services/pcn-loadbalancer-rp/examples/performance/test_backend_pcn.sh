source "${BASH_SOURCE%/*}/../helpers.bash"

set -x

function cleanup {
  set +e
  polycubectl simplebridge del br1
  polycubectl router del r1
  delete_veth 4
}


trap cleanup EXIT

echo '	##### SETTING TEST ENVIROMENT ######'

create_veth 4

polycubectl simplebridge add br1  #loglevel=TRACE #type=XDP_SKB
polycubectl lbrp add lb1 loglevel=TRACE #type=XDP_SKB

#config lb1
polycubectl lbrp lb1 ports add veth1 peer=veth1 type=FRONTEND #BACKEND 0
polycubectl lbrp lb1 ports add p1 peer=lb1	type=BACKEND #FRONTEND 1

polycubectl simplebridge br1 ports add p1
polycubectl simplebridge br1 ports add p2
polycubectl simplebridge br1 ports add p3
polycubectl simplebridge br1 ports add p4
polycubectl simplebridge br1 ports add p5
polycubectl simplebridge br1 ports add p6


polycubectl simplebridge br1 ports p1 set peer=enx000ec6fe1bfc
polycubectl simplebridge br1 ports p2 set peer=veth1
polycubectl simplebridge br1 ports p3 set peer=veth2
polycubectl simplebridge br1 ports p4 set peer=veth3
polycubectl simplebridge br1 ports p6 set peer=veth4



polycubectl router add r1 #loglevel=TRACE #type=XDP_SKB
polycubectl router r1 ports add p5 netmask=255.255.255.0 ip=10.0.0.254
#polycubectl router r1 ports p5 secondaryip add  20.0.0.254 255.255.255.0
polycubectl router r1 ports p5 set peer=br1:p5

polycubectl simplebridge br1 ports p5 set peer=r1:p5



polycubectl lbrp lb1 service add 192.168.0.1 180 ALL name=service1
polycubectl lbrp lb1 service 192.168.0.1 180 TCP backend add 10.0.0.4 name=B4 port=80
polycubectl lbrp lb1 service 192.168.0.1 180 TCP backend add 10.0.0.3 name=B3 port=80
polycubectl lbrp lb1 service 192.168.0.1 180 TCP backend add 10.0.0.2 name=B2 port=80
polycubectl lbrp lb1 service 192.168.0.1 180 TCP backend add 10.0.0.1 name=B1 port=80 weight=0
polycubectl lbrp lb1 service 192.168.0.1 180 UDP backend add 10.0.0.4 name=B4 port=80
polycubectl lbrp lb1 service 192.168.0.1 180 UDP backend add 10.0.0.3 name=B3 port=80
polycubectl lbrp lb1 service 192.168.0.1 180 UDP backend add 10.0.0.2 name=B2 port=80
polycubectl lbrp lb1 service 192.168.0.1 180 UDP backend add 10.0.0.1 name=B1 port=80 weight=0
polycubectl lbrp lb1 service 192.168.0.1 0 ICMP backend add 10.0.0.4 name=B4 port=80
polycubectl lbrp lb1 service 192.168.0.1 0 ICMP backend add 10.0.0.3 name=B3 port=80
polycubectl lbrp lb1 service 192.168.0.1 0 ICMP backend add 10.0.0.2 name=B2 port=80
polycubectl lbrp lb1 service 192.168.0.1 0 ICMP backend add 10.0.0.1 name=B1 port=80 weight=0

echo '	##### STARTING 2 SERVERS IN NS2 AND NS3 ######'

sudo ip netns exec ns1 node s.js &
sudo ip netns exec ns2 node s.js &
sudo ip netns exec ns3 node s.js &
sudo ip netns exec ns4 node s.js &


read
