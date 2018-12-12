source "${BASH_SOURCE%/*}/helpers.bash"

set -x

function cleanup {
  set +e
  polycubectl simplebridge del br1

  delete_veth 2
}


trap cleanup EXIT

echo '	##### SETTING TEST ENVIROMENT ######'

create_veth 2

polycubectl simplebridge add br1 loglevel=TRACE type=XDP_SKB

polycubectl simplebridge br1 ports add p1
polycubectl simplebridge br1 ports add p2
polycubectl simplebridge br1 ports p1 set peer=veth1
polycubectl simplebridge br1 ports p2 set peer=veth2


#config lb2


echo '	##### STARTING 2 SERVERS IN NS2 AND NS3 ######'

sleep 1000000







