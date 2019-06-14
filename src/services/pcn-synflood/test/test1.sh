function synfloodcleanup {
  set +e
  polycubectl synflood del sf
}
trap synfloodcleanup EXIT

echo -e '\nTest 1 \n'
set -e
set -x

polycubectl synflood add sf
polycubectl sf show
polycubectl sf stats show

polycubectl sf stats tcpattemptfails show
polycubectl sf stats tcpoutrsts show
polycubectl sf stats deliverratio show
polycubectl sf stats responseratio show
polycubectl sf stats lastupdate show
