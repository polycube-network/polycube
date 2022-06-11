#!/bin/bash

# This script compiles and installs polycube and its dependencies

function print_system_info {
  echo "***********************SYSTEM INFO*************************************"
  echo "kernel version:" $(uname -r)
  echo "linux release info:" $(cat /etc/*-release)
  echo "g++ version:" $(g++ --version)
  echo "gcc version:" $(gcc --version)
  echo "golang-version:" $(go version)
  # TODO: what else is important to gather?
}

function error_message {
  set +x
  echo
  echo 'Error during installation'
  print_system_info
  exit 1
}
function success_message {
  set +x
  echo
  echo 'Installation completed successfully'
  echo
  echo 'You can now start the polycube daemon:'
  echo '  manually:     sudo polycubed -d'
  echo '  with systemd: sudo systemctl start polycubed'
  echo 'and then interact with it using the Polycube command line:'
  echo '  polycubectl -h'
  echo
  exit 0
}
trap error_message ERR

function show_help() {
usage="$(basename "$0") [mode]
Polycube installation script

mode:
  pcn-iptables: install only pcn-iptables service and related components
  pcn-k8s: install only pcn-k8s (only to be used with docker build)
  update: updates the source code of polycube before *default* install
  default: install all available polycube services"
echo "$usage"
echo
}

while getopts h option; do
 case "${option}" in
 h|\?)
  show_help
  exit 0
 esac
done

echo "Use 'install.sh -h' to show advanced installation options."

MODE=$1

[ -z ${SUDO+x} ] && SUDO='sudo'
[ -z ${WORKDIR+x} ] && WORKDIR=$HOME/dev
[ -z ${MODE+x} ] && MODE='default'

# print bash commands and their arguments as they are executed
set -x
# exit immediately if a command exits with a non-zero status
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source scripts/pre-requirements.sh

echo "Install Prometheus-cpp"
cd $DIR/../src/libs/prometheus-cpp
set +e
if [ -d _build ]; then
    $SUDO rm -rf _build
fi
mkdir _build
cd _build
cmake .. -DBUILD_SHARED_LIBS=ON
make -j $(getconf _NPROCESSORS_ONLN)
$SUDO make install


echo "Install polycube"
cd $DIR/..
if [ "$INSTALL_CLEAN_POLYCUBE" == true ] ; then
  $SUDO rm -rf build
fi

if [ "$MODE" == "update" ] ; then
  echo "Update to the latest polycube source code"
  # Commands below update also the code in this repository and
  # all the submodules, so that you install the most recent code
  # from the current branch (default: master)
  git pull
  git submodule update --init --recursive

  # Print the last commit in the current branch, just to make sure
  # we're compiling exactly what we expect
  git log -1
fi

mkdir -p build && cd build

# depending on the mode different services are enabled
if [ "$MODE" == "pcn-iptables" ]; then
  cmake .. -DENABLE_PCN_IPTABLES=ON \
    -DENABLE_SERVICE_BRIDGE=OFF \
    -DENABLE_SERVICE_DDOSMITIGATOR=OFF \
    -DENABLE_SERVICE_FIREWALL=OFF \
    -DENABLE_SERVICE_HELLOWORLD=OFF \
    -DENABLE_SERVICE_IPTABLES=ON \
    -DENABLE_SERVICE_K8SFILTER=OFF \
    -DENABLE_SERVICE_K8SWITCH=OFF \
    -DENABLE_SERVICE_LBDSR=OFF \
    -DENABLE_SERVICE_LBRP=OFF \
    -DENABLE_SERVICE_NAT=OFF \
    -DENABLE_SERVICE_PBFORWARDER=OFF \
    -DENABLE_SERVICE_ROUTER=OFF \
    -DENABLE_SERVICE_SIMPLEBRIDGE=OFF \
    -DENABLE_SERVICE_SIMPLEFORWARDER=OFF \
    -DENABLE_SERVICE_TRANSPARENTHELLOWORLD=OFF \
    -DENABLE_SERVICE_SYNFLOOD=OFF \
    -DENABLE_SERVICE_PACKETCAPTURE=OFF \
    -DENABLE_SERVICE_K8SLBRP=OFF \
    -DENABLE_SERVICE_K8SDISPATCHER=OFF
elif [ "$MODE" == "pcn-k8s" ]; then
  cmake .. -DENABLE_SERVICE_BRIDGE=OFF \
    -DENABLE_SERVICE_DDOSMITIGATOR=ON \
    -DENABLE_SERVICE_FIREWALL=ON \
    -DENABLE_SERVICE_HELLOWORLD=OFF \
    -DENABLE_SERVICE_IPTABLES=OFF \
    -DENABLE_SERVICE_K8SFILTER=ON \
    -DENABLE_SERVICE_K8SWITCH=ON \
    -DENABLE_SERVICE_LBDSR=OFF \
    -DENABLE_SERVICE_LBRP=OFF \
    -DENABLE_SERVICE_NAT=OFF \
    -DENABLE_SERVICE_PBFORWARDER=OFF \
    -DENABLE_SERVICE_ROUTER=OFF \
    -DENABLE_SERVICE_SIMPLEBRIDGE=OFF \
    -DENABLE_SERVICE_SIMPLEFORWARDER=OFF \
    -DENABLE_SERVICE_TRANSPARENTHELLOWORLD=OFF \
    -DENABLE_SERVICE_SYNFLOOD=OFF \
    -DENABLE_SERVICE_PACKETCAPTURE=ON \
    -DENABLE_SERVICE_K8SLBRP=OFF \
    -DENABLE_SERVICE_K8SDISPATCHER=OFF
else
  cmake .. -DENABLE_PCN_IPTABLES=ON
fi
make -j $(getconf _NPROCESSORS_ONLN)
$SUDO make install

success_message
