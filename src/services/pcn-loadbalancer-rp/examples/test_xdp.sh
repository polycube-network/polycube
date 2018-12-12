source "${BASH_SOURCE%/*}/helpers.bash"

set -x

function cleanup {
  set +e
  polycubectl simplebridge del br1
  polycubectl lbrp del lb1
  polycubectl lbrp del lb2
  polycubectl router del r1

  delete_veth 2
}


trap cleanup EXIT

echo '	##### SETTING TEST ENVIROMENT ######'

create_veth 2
#create_link 3

polycubectl simplebridge add br1 loglevel=TRACE type=XDP_SKB
polycubectl lbrp add lb1 loglevel=TRACE type=XDP_SKB
polycubectl lbrp add lb2 loglevel=TRACE type=XDP_SKB


polycubectl simplebridge br1 ports add p1
polycubectl simplebridge br1 ports add p2
#polycubectl simplebridge br1 ports add p3


#sudo ip netns exec ns1 ifconfig veth1_:new_range 20.0.0.1 up


#config lb1

polycubectl lbrp lb1 ports add veth1 peer=veth1 type=FRONTEND #BACKEND 0
polycubectl lbrp lb1 ports add p1 peer=br1:p1	type=BACKEND #FRONTEND 1
polycubectl simplebridge br1 ports p1 set peer=lb1:p1

#config lb2
polycubectl lbrp lb2 ports add veth2 peer=veth2 type=FRONTEND #BACKEND 0
polycubectl lbrp lb2 ports add p2 peer=br1:p2 type=BACKEND #FRONTEND 1
polycubectl simplebridge br1 ports p2 set peer=lb2:p2


# polycubectl router add r1 loglevel=TRACE type=XDP_SKB
# polycubectl router r1 ports add p4 netmask=255.255.255.0 ip=10.0.0.254
# polycubectl router r1 ports p4 set peer=br1:p3
# polycubectl simplebridge br1 ports p3 set peer=r1:p4


#sudo ip netns exec ns2 route add default gw 10.0.0.254 veth2_
#sudo ip netns exec ns1 route add default gw 10.0.0.254 veth1_



echo '	##### ADD SERVICE AND BACKENDS ######'

polycubectl lbrp lb1 service add 192.168.0.1 180 ALL name=service1
#polycubectl lbrp lb1 src-ip-rewrite set ip-range=10.0.0.1/24 new_ip_range=20.0.0.1/24
polycubectl lbrp lb1 service 192.168.0.1 180 TCP backend add 10.0.0.2 name=B2 port=80
polycubectl lbrp lb1 service 192.168.0.1 180 TCP backend add 10.0.0.1 name=B1 port=80 weight=0
polycubectl lbrp lb1 service 192.168.0.1 180 UDP backend add 10.0.0.2 name=B2 port=80
polycubectl lbrp lb1 service 192.168.0.1 180 UDP backend add 10.0.0.1 name=B1 port=80 weight=0
polycubectl lbrp lb1 service 192.168.0.1 0 ICMP backend add 10.0.0.2 name=B2 port=80
polycubectl lbrp lb1 service 192.168.0.1 0 ICMP backend add 10.0.0.1 name=B1 port=80 weight=0

polycubectl lbrp lb2 service add 192.168.0.1 180 ALL name=service1
#polycubectl lbrp lb2 src-ip-rewrite set ip-range=10.0.0.1/24 new_ip_range=20.0.0.1/24
polycubectl lbrp lb2 service 192.168.0.1 180 TCP backend add 10.0.0.2 name=B2 port=80
polycubectl lbrp lb2 service 192.168.0.1 180 TCP backend add 10.0.0.1 name=B1 port=80 weight=0
polycubectl lbrp lb2 service 192.168.0.1 180 UDP backend add 10.0.0.2 name=B2 port=80
polycubectl lbrp lb2 service 192.168.0.1 180 UDP backend add 10.0.0.1 name=B1 port=80 weight=0
polycubectl lbrp lb2 service 192.168.0.1 0 ICMP backend add 10.0.0.2 name=B2 port=80
polycubectl lbrp lb2 service 192.168.0.1 0 ICMP backend add 10.0.0.1 name=B1 port=80 weight=0



echo '	##### STARTING 2 SERVERS IN NS2 AND NS3 ######'


sudo ip netns exec ns2 python server2.py &


sleep 1


sleep 1000000




