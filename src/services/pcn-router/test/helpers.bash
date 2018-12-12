#!/usr/bin/env bash

function test_fail {
  set +e
  res=$($@)
  local status=$?
  set -e
  if [ $status -ne 0 ]; then
    return 0
  else
    return 1
  fi
}

function add_routers {
  for i in `seq 1 $1`;
  do
    polycubectl router add r$i loglevel=DEBUG
  done
}

function del_routers {
  for i in `seq 1 $1`;
  do
    polycubectl router del r$i
  done
}

function router_add_port_as_gateway { #$3 network number #$4 bridge name
  polycubectl router $1 ports add to_$2 ip=10.0.$3.254 netmask=255.255.255.0
  if [[ $# -eq 4 ]] ; then
    polycubectl simplebridge $4 ports add to_$1
    polycubectl connect $4:to_$1 $1:to_$4
  else
    echo "Executing the command 'polycubectl router $1 ports to_$2 set peer=$2'"
    polycubectl router $1 ports to_$2 set peer=$2
  fi

}

function router_add_port { #$3 ip $4 netmask
  polycubectl router $1 ports add $2 ip=$3 netmask=$4
}

function connect_router_p_to_p { #$1 router name 1, $2 router name 2, $3 network point to point connection
  polycubectl router $1 ports add to_$2 ip=10.$3.0.1 netmask=255.255.255.252
  polycubectl router $2 ports add to_$1 ip=10.$3.0.2 netmask=255.255.255.252
  polycubectl connect $1:to_$2 $2:to_$1
}

function connect_router_secondary_p_to_p { #$1 router name 1, $2 router name 2, $3 network point to point connection
  polycubectl router $1 ports to_$2 secondaryip add 10.$3.0.1 255.255.255.252
  polycubectl router $2 ports to_$1 secondaryip add 10.$3.0.2 255.255.255.252
  polycubectl connect $1:to_$2 $2:to_$1
}



function router_add_secondary {
  polycubectl router $1 ports to_$2 secondaryip add $3 $4
}

function router_add_secondary_as_gateway {
polycubectl router $1 ports to_$2 secondaryip add 10.0.$3.254 255.255.255.0
  if [[ $# -eq 4 ]] ; then
    polycubectl simplebridge $4 ports add to_$1
    polycubectl connect $4:to_$1 $1:to_$4
  else
    polycubectl router $1 ports to_$2 set peer=$2
  fi
}

function router_add_route {

  if [[ $# -eq 5 ]] ; then
    polycubectl router $1 route add $2 $3 $4 pathcost=$5
  else
  	polycubectl router $1 route add $2 $3 $4
  fi

}

function router_routingtable_show {
polycubectl router $1 route show
}


function ping_special {
for i in `seq 1 $1`;
  do
    for ((j = 2; j <= $#; j++));
     do
        sudo ip netns exec ns$i ping ${!j} -c 2 -i 0.5
     done
  done
}

function ping_special_fail {
for i in `seq 1 $1`;
  do
    for ((j = 2; j <= $#; j++));
     do
        test_fail sudo ip netns exec ns$i ping ${!j} -c 2 -i 0.5 -w 1
     done
  done
}

function ping_cycle {
  for i in `seq 1 $1`;
  do
    for j in `seq 1 $1`;
    do
      if [ "$i" -ne "$j" ]; then
        sudo ip netns exec ns$i ping 10.0.$j.1 -c 2 -i 0.5
      fi
    done
  done
}

function ping_cycle_subnet {
  for i in `seq 1 $1`;
  do

      count=1
       net=1

  for j in `seq 1 $1`;
    do

      if [ "$i" -ne "$j" ]; then
        sudo ip netns exec ns$i ping 10.0.${net}.${count} -c 2 -i 0.5
      fi

      if [ $count -ge ${2} ]; then
          net=$((net+1))
          count=0
      fi
      count=$((count+1))

    done

  done
}


function create_veth_net {
  for i in `seq 1 $1`;
  do
  	sudo ip netns add ns${i}
  	sudo ip link add veth${i}_ type veth peer name veth${i}
  	sudo ip link set veth${i}_ netns ns${i}
  	sudo ip netns exec ns${i} ip link set dev veth${i}_ up
  	sudo ip link set dev veth${i} up
        sudo ip netns exec ns${i} ifconfig veth${i}_ 10.0.${i}.1/24
        sudo ip netns exec ns${i} route add default gw 10.0.${i}.254 veth${i}_

  done
}

function create_veth_subnet {  #$1 n of host $2 host of the same net
  count=1
  net=1
  for i in `seq 1 $1`;
  do
  	sudo ip netns add ns${i}
  	sudo ip link add veth${i}_ type veth peer name veth${i}
  	sudo ip link set veth${i}_ netns ns${i}
  	sudo ip netns exec ns${i} ip link set dev veth${i}_ up
  	sudo ip link set dev veth${i} up
        sudo ip netns exec ns${i} ifconfig veth${i}_ 10.0.${net}.${count}/24
        sudo ip netns exec ns${i} route add default gw 10.0.${net}.254 veth${i}_
        if [ $count -ge ${2} ]; then
          ((net++))
          count=0
        fi
        ((count++))

  done
}

function create_link {
  for i in `seq 1 $1`;
  do
  	sudo ip link add link${i}1 type veth peer name link${i}2
  	sudo ip link set dev link${i}1 up
  	sudo ip link set dev link${i}2 up
  done
}

function delete_veth {
  for i in `seq 1 $1`;
  do
  	sudo ip link del veth${i}
  	sudo ip netns del ns${i}
  done
}

function delete_link {
  for i in `seq 1 $1`;
  do
  	sudo ip link del link${i}1
  done
}
