echo "		#####Test#####		"

polycubectl lbrp add lb1 loglevel=TRACE
polycubectl lbrp lb1 service add 192.168.0.1 80 TCP name=service_1
polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend add 10.0.0.1 name=backend1 port=80 weight=20
polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend add 10.0.0.2 name=backend2 port=80 weight=30
polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend add 10.0.0.3 name=backend3 port=80 weight=20
polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend add 10.0.0.4 name=backend4 port=80

sleep 10

polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend add 10.0.0.5 name=backend5 port=80 weight=100
polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend add 10.0.0.6 name=backend6 port=80 weight=40

sleep 10

polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend add 10.0.0.7 name=backend7 port=80
polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend add 10.0.0.8 name=backend8 port=80 weight=20

sleep 10

polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend del 10.0.0.2
polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend del 10.0.0.5
polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend del 10.0.0.1
polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend del 10.0.0.3
polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend del 10.0.0.4
polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend del 10.0.0.6
polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend del 10.0.0.8
polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend del 10.0.0.7


sleep 10

polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend show
polycubectl lbrp lb1 service 192.168.0.1 80 TCP show



polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend add 10.0.0.3 name=backend3 port=80 weight=20
polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend add 10.0.0.4 name=backend4 port=80

sleep 10

polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend add 10.0.0.5 name=backend5 port=80 weight=60
polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend add 10.0.0.6 name=backend6 port=80 weight=40

polycubectl lbrp lb1 service 192.168.0.1 80 TCP backend show
polycubectl lbrp lb1 service 192.168.0.1 80 TCP show

sleep 5

polycubectl lbrp lb1 service del 192.168.0.1 80 TCP

polycubectl lbrp lb1 show






