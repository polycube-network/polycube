#! /bin/bash

# check that the services are actually transverse on the right order

source "${BASH_SOURCE%/*}/../helpers.bash"

set -e
set -x

function cleanup {
  set +e
  polycubectl del th1
  polycubectl del th2
  polycubectl del th3
  polycubectl del th4
  polycubectl del th5
  delete_veth 1
  echo "FAIL"
}
trap cleanup EXIT

create_veth 1

polycubectl transparenthelloworld add th1 loglevel=TRACE
polycubectl transparenthelloworld add th2 loglevel=TRACE
polycubectl transparenthelloworld add th3 loglevel=TRACE
polycubectl transparenthelloworld add th4 loglevel=TRACE
polycubectl transparenthelloworld add th5 loglevel=TRACE

polycubectl attach th3 veth1
polycubectl attach th1 veth1 position=first
polycubectl attach th5 veth1 position=last
polycubectl attach th2 veth1 after=th1
polycubectl attach th4 veth1 before=th5

# verfiy the service chain order
polycubectl topology show veth1 | egrep "ingress service chain: th1 ---> th2 ---> th3 ---> th4 ---> th5 ---> stack"
polycubectl topology show veth1 | egrep "egress service chain: th5 ---> th4 ---> th3 ---> th2 ---> th1 ---> stack"

# sudo ip netns exec ns1 ping 10.0.0.2 -c 1

#TODO: verify the log output, expected log output
#[2019-12-06 11:39:20.512] [Transparenthelloworld] [th1] [debug] Ingress: passing packet
#[2019-12-06 11:39:20.512] [Transparenthelloworld] [th2] [debug] Ingress: passing packet
#[2019-12-06 11:39:20.512] [Transparenthelloworld] [th3] [debug] Ingress: passing packet
#[2019-12-06 11:39:20.512] [Transparenthelloworld] [th4] [debug] Ingress: passing packet
#[2019-12-06 11:39:20.512] [Transparenthelloworld] [th5] [debug] Ingress: passing packet
#[2019-12-06 11:39:20.512] [Transparenthelloworld] [th5] [debug] Egress: passing packet
#[2019-12-06 11:39:20.512] [Transparenthelloworld] [th4] [debug] Egress: passing packet
#[2019-12-06 11:39:20.512] [Transparenthelloworld] [th3] [debug] Egress: passing packet
#[2019-12-06 11:39:20.512] [Transparenthelloworld] [th2] [debug] Egress: passing packet
#[2019-12-06 11:39:20.512] [Transparenthelloworld] [th1] [debug] Egress: passing packet

polycubectl del th1
polycubectl del th2
polycubectl del th3
polycubectl del th4
polycubectl del th5
delete_veth 1

set +x
trap - EXIT
echo "SUCCESS"
