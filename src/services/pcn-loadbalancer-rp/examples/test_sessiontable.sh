source "${BASH_SOURCE%/*}/helpers.bash"

set -x

function cleanup {
  set +e
  polycubectl simplebridge del br1
  polycubectl lbrp del lb1
  polycubectl lbrp del lb2
  polycubectl lbrp del lb2
  polycubectl router del r1

  delete_veth 3
  delete_link 4
}


trap cleanup EXIT

echo '	##### SETTING TEST ENVIROMENT ######'

create_veth 3
create_link 4

polycubectl simplebridge add br1 #type=XDP_SKB
polycubectl lbrp add lb1 loglevel=TRACE #type=XDP_SKB
polycubectl lbrp add lb2 loglevel=TRACE #type=XDP_SKB
polycubectl lbrp add lb3 loglevel=TRACE #type=XDP_SKB


#loadbalancer ports
bridge_add_port br1 link12
bridge_add_port br1 link22
bridge_add_port br1 link32
bridge_add_port br1 link42



#config lb1

polycubectl lbrp lb1 ports add veth1 peer=veth1 type=FRONTEND #BACKEND 0

polycubectl lbrp lb1 ports add link11 peer=link11	type=BACKEND #FRONTEND 1


#config lb2
polycubectl lbrp lb2 ports add veth2 peer=veth2 type=FRONTEND #BACKEND 0

polycubectl lbrp lb2 ports add link21 peer=link21	type=BACKEND #FRONTEND 1


#config lb3
polycubectl lbrp lb3 ports add veth3 peer=veth3 type=FRONTEND #BACKEND 0

polycubectl lbrp lb3 ports add link31	peer=link31 type=BACKEND #FRONTEND 1

polycubectl router add r1
polycubectl router r1 ports add link41 netmask=255.255.255.0 ip=10.0.0.254
polycubectl router r1 ports link41 set peer=link41

sudo ip netns exec ns3 route add default gw 10.0.0.254 veth3_

echo '	##### ADD SERVICE AND BACKENDS ######'

polycubectl lbrp lb1 service add 192.168.0.1 80 ALL name=service1
polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend add 10.0.0.1 name=B1 port=80 weight=50
polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend add 10.0.0.2 name=B2 port=80
polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend add 10.0.0.3 name=B3 port=80 weight=0

polycubectl lbrp lb1 service 192.168.0.1 80 UDP backend add 10.0.0.1 name=B1 port=80 weight=50
polycubectl lbrp lb1 service 192.168.0.1 80 UDP backend add 10.0.0.2 name=B2 port=80
polycubectl lbrp lb1 service 192.168.0.1 80 UDP backend add 10.0.0.3 name=B3 port=80 weight=0

polycubectl lbrp lb1 service 192.168.0.1 0 ICMP backend add 10.0.0.1 name=B1 port=80 weight=50
polycubectl lbrp lb1 service 192.168.0.1 0 ICMP backend add 10.0.0.2 name=B2 port=80
polycubectl lbrp lb1 service 192.168.0.1 0 ICMP backend add 10.0.0.3 name=B3 port=80 weight=0


polycubectl lbrp lb2 service add 192.168.0.1 80 ALL name=service1
polycubectl lbrp lb2 service 192.168.0.1 80 TCP backend add 10.0.0.1 name=B1 port=80 weight=50
polycubectl lbrp lb2 service 192.168.0.1 80 TCP backend add 10.0.0.2 name=B2 port=80
polycubectl lbrp lb2 service 192.168.0.1 80 TCP backend add 10.0.0.3 name=B3 port=80 weight=0

polycubectl lbrp lb2 service 192.168.0.1 80 UDP backend add 10.0.0.1 name=B1 port=80 weight=50
polycubectl lbrp lb2 service 192.168.0.1 80 UDP backend add 10.0.0.2 name=B2 port=80
polycubectl lbrp lb2 service 192.168.0.1 80 udp backend add 10.0.0.3 name=B3 port=80 weight=0

polycubectl lbrp lb2 service 192.168.0.1 0 ICMP backend add 10.0.0.1 name=B1 port=80 weight=50
polycubectl lbrp lb2 service 192.168.0.1 0 ICMP backend add 10.0.0.2 name=B2 port=80
polycubectl lbrp lb2 service 192.168.0.1 0 ICMP backend add 10.0.0.3 name=B3 port=80 weight=0


polycubectl lbrp lb3 service add 192.168.0.1 80 ALL name=service1
polycubectl lbrp lb3 service 192.168.0.1 80 TCP backend add 10.0.0.1 name=B1 port=80 weight=50
polycubectl lbrp lb3 service 192.168.0.1 80 TCP backend add 10.0.0.2 name=B2 port=80
polycubectl lbrp lb3 service 192.168.0.1 80 TCP backend add 10.0.0.3 name=B3 port=80 weight=0

polycubectl lbrp lb3 service 192.168.0.1 80 UDP backend add 10.0.0.1 name=B1 port=80 weight=50
polycubectl lbrp lb3 service 192.168.0.1 80 UDP backend add 10.0.0.2 name=B2 port=80
polycubectl lbrp lb3 service 192.168.0.1 80 UDP backend add 10.0.0.3 name=B3 port=80 weight=0

polycubectl lbrp lb3 service 192.168.0.1 0 ICMP backend add 10.0.0.1 name=B1 port=80 weight=50
polycubectl lbrp lb3 service 192.168.0.1 0 ICMP backend add 10.0.0.2 name=B2 port=80
polycubectl lbrp lb3 service 192.168.0.1 0 ICMP backend add 10.0.0.3 name=B3 port=80 weight=0


echo '	##### STARTING 2 SERVERS IN NS2 AND NS3 ######'


sudo ip netns exec ns1 python server1.py &

sleep 1

sudo ip netns exec ns2 python server2.py &

sleep 1



echo '	#####  TEST ALL ######'

for i in `seq 1 1`;
do
  for j in `seq 1 100`;
  do
    sudo ip netns exec ns3 wget -qO- 192.168.0.$i:80 &> /dev/null
  done
done

echo '	##### TEST ICMP BACKENDS ######'

sudo ip netns exec ns1 ping 10.0.0.2 -c 5
sudo ip netns exec ns2 ping 10.0.0.1 -c 5
sudo ip netns exec ns3 ping 10.0.0.2 -c 5
sudo ip netns exec ns1 ping 10.0.0.3 -c 5
#sudo ip netns exec ns3 ping 192.168.0.1 -c 5



sleep 1

echo '	##### TEST ENVIROMENT DELETION BACKEND 10.0.0.2######'

polycubectl lbrp lb1 service 192.168.0.1 80 ALL backend del 10.0.0.2
polycubectl lbrp lb2 service 192.168.0.1 80 ALL backend del 10.0.0.2
polycubectl lbrp lb3 service 192.168.0.1 80 ALL backend del 10.0.0.2


sleep 1

echo '	#####  TEST ALL ######'

for i in `seq 1 1`;
do
  for j in `seq 1 10`;
  do
    sudo ip netns exec ns3 wget -qO- 192.168.0.$i:80 &> /dev/null
  done
done


echo '	#####  TEST ICMP BACKEND ######'

sudo ip netns exec ns1 ping 10.0.0.3 -c 5
#sudo ip netns exec ns3 ping 192.168.0.1 -c 5


sleep 2

echo "##### TEST ICMP SERVICE #####"

polycubectl lbrp lb3 service add 33.33.66.7 0 ICMP name=Service33
polycubectl lbrp lb3 service 33.33.66.7 0 ICMP backend add 10.0.0.1 name=B1 port=80

sudo ip netns exec ns3 ping 33.33.66.7 -c 5


echo '	##### ADD SERVICE AND BACKENDS ######'

polycubectl lbrp lb1 service 192.168.0.1 80 ALL backend add 10.0.0.2 name=B2 port=80 weight=100

polycubectl lbrp lb2 service 192.168.0.1 80 ALL backend add 10.0.0.2 name=B2 port=80 weight=100

polycubectl lbrp lb3 service 192.168.0.1 80 ALL backend add 10.0.0.2 name=B2 port=80 weight=100


polycubectl lbrp show

echo '	##### Press ctrl+c to terminate ##### '
sleep 1000000




