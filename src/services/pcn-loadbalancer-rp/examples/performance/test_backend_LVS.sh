source "${BASH_SOURCE%/*}/../helpers.bash"

set -x

function cleanup {
  set +e
  delete_veth 4
  sudo ifconfig b down
  sudo brctl delbr b
}


trap cleanup EXIT

echo '	##### SETTING TEST ENVIROMENT ######'

create_veth 4

#sudo ip link add d0 type dummy0
#sudo ifconfig d0 up
#sudo ip link add lb1 type veth peer name lb2

sudo ifconfig lb1 up
sudo ifconfig lb2 up

sudo brctl addbr b
sudo brctl addif b lb2
sudo brctl addif b veth2
sudo brctl addif b veth3
sudo brctl addif b veth4
#sudo brctl addif b enx000ec6fe1bfc

sudo ifconfig b 10.0.0.254 up


#sudo ip netns exec ns1 node s.js &
sudo ip netns exec ns2 node s.js &
sudo ip netns exec ns3 node s.js &
sudo ip netns exec ns4 node s.js &

sleep 2

read
