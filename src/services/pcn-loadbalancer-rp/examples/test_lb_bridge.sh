source "${BASH_SOURCE%/*}/helpers.bash"

set -x

function cleanup {
  set +e
  polycubectl simplebridge del br1
  polycubectl lbrp del lb1
  polycubectl router del r1

  delete_veth 4
}


trap cleanup EXIT

echo '	##### SETTING TEST ENVIROMENT ######'

create_veth 4
#create_link 3

polycubectl simplebridge add br1 loglevel=TRACE type=XDP_SKB
polycubectl lbrp add lb1 loglevel=TRACE type=XDP_SKB


polycubectl simplebridge br1 ports add p1
polycubectl simplebridge br1 ports add p2
polycubectl simplebridge br1 ports add p3
polycubectl simplebridge br1 ports add p4


#config lb1
polycubectl lbrp lb1 ports add p1 type=FRONTEND #BACKEND 0
polycubectl lbrp lb1 ports add p2	type=BACKEND #FRONTEND 1
polycubectl lbrp lb1 ports p2 set peer=br1:p1
polycubectl lbrp lb1 ports p1 set peer=veth1  #FRONTEND 1

polycubectl simplebridge br1 ports p1 set peer=lb1:p2
polycubectl simplebridge br1 ports p2 set peer=veth2
polycubectl simplebridge br1 ports p3 set peer=veth3
polycubectl simplebridge br1 ports p4 set peer=veth4




echo '	##### ADD SERVICE AND BACKENDS ######'

# polycubectl lbrp lb1 service add 192.168.0.1 180 ALL name=service1
# #polycubectl lbrp lb1 src-ip-rewrite set ip-range=10.0.0.1/24 new_ip_range=20.0.0.1/24
# polycubectl lbrp lb1 service 192.168.0.1 180 TCP backend add 10.0.0.2 name=B2 port=80
# polycubectl lbrp lb1 service 192.168.0.1 180 TCP backend add 10.0.0.1 name=B1 port=80 weight=0
# polycubectl lbrp lb1 service 192.168.0.1 180 UDP backend add 10.0.0.2 name=B2 port=80
# polycubectl lbrp lb1 service 192.168.0.1 180 UDP backend add 10.0.0.1 name=B1 port=80 weight=0
# polycubectl lbrp lb1 service 192.168.0.1 0 ICMP backend add 10.0.0.2 name=B2 port=80
# polycubectl lbrp lb1 service 192.168.0.1 0 ICMP backend add 10.0.0.1 name=B1 port=80 weight=0



echo '	##### STARTING 2 SERVERS IN NS2 AND NS3 ######'


# sudo ip netns exec ns2 python server2.py &


sleep 1


sleep 1000000




