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
  exit 0
}
trap error_message ERR

function show_help() {
usage="$(basename "$0") [option]
Polycube installation script

option:
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

# script's path
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

mkdir -p $WORKDIR

$SUDO apt update
$SUDO apt install -y wget gnupg2 software-properties-common

# golang v1.12 still not available in repos
$SUDO add-apt-repository ppa:longsleep/golang-backports -y

# repo for libyang-dev
$SUDO sh -c "echo 'deb http://download.opensuse.org/repositories/home:/liberouter/xUbuntu_18.04/ /' > /etc/apt/sources.list.d/home:liberouter.list"
wget -nv https://download.opensuse.org/repositories/home:liberouter/xUbuntu_18.04/Release.key -O Release.key
$SUDO apt-key add - < Release.key
$SUDO apt update

PACKAGES=""
PACKAGES+=" git" # needed to clone dependencies
PACKAGES+=" build-essential cmake" # provides compiler and other compilation tools
PACKAGES+=" bison flex libelf-dev" # bcc dependencies
PACKAGES+=" libllvm5.0 llvm-5.0-dev libclang-5.0-dev" # bpf tools compilation tool chain
PACKAGES+=" libnl-route-3-dev libnl-genl-3-dev" # netlink library
PACKAGES+=" uuid-dev"
PACKAGES+=" golang-go" # needed for polycubectl and pcn-k8s
PACKAGES+=" pkg-config"
PACKAGES+=" libyang-dev"
PACKAGES+=" autoconf libtool m4 automake"
PACKAGES+=" libssl-dev" # needed for certificate based security
PACKAGES+=" sudo" # needed for pcn-iptables, when building docker image
PACKAGES+=" kmod" # needed for pcn-iptables, when using lsmod to unload conntrack if not needed
PACKAGES+=" jq bash-completion" # needed for polycubectl bash autocompletion

if [ "$MODE" == "pcn-k8s" ]; then
  PACKAGES+=" curl" # needed for pcn-k8s to download a binary
  PACKAGES+=" iptables" # only for pcn-k8s
  PACKAGES+=" iproute2" # provides bridge command that is used to add entries in vxlan device
fi

# use non inreractive to avoid blocking the install script
$SUDO bash -c "DEBIAN_FRONTEND=noninteractive apt-get install -yq $PACKAGES"

echo "Install pistache"
cd $WORKDIR
set +e
if [ ! -d pistache ]; then
  git clone https://github.com/oktal/pistache.git
fi

cd pistache
# use last known working version
git checkout 117db02eda9d63935193ad98be813987f6c32b33
git submodule update --init
mkdir -p build && cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DPISTACHE_USE_SSL=ON ..
make -j $(getconf _NPROCESSORS_ONLN)
$SUDO make install

echo "Install libtins"
cd $WORKDIR
set +e
if [ ! -d libtins ]; then
  git clone --branch v3.5 https://github.com/mfontanini/libtins.git
fi

cd libtins
mkdir -p build && cd build
cmake -DLIBTINS_ENABLE_CXX11=1 \
  -DLIBTINS_BUILD_EXAMPLES=OFF -DLIBTINS_BUILD_TESTS=OFF \
  -DLIBTINS_ENABLE_DOT11=OFF -DLIBTINS_ENABLE_PCAP=OFF \
  -DLIBTINS_ENABLE_WPA2=OFF -DLIBTINS_ENABLE_WPA2_CALLBACKS=OFF ..
make -j $(getconf _NPROCESSORS_ONLN)
$SUDO make install
$SUDO ldconfig

# Set $GOPATH, if not already set
if [[ -z "${GOPATH}" ]]; then
  mkdir -p $HOME/go
  export GOPATH=$HOME/go
fi

echo "Install polycube"
cd $DIR/..
if [ "$BUILD_CLEAN" == true ] ; then
  $SUDO rm -rf build
fi

if [ "$MODE" == "update" ] ; then
  echo "Update to the latest polycube source code"
  # Commands below update also the code in this repository and
  # all the submodules, so that you install the most recent code
  # from the current branch (default: master)
  git pull
  git submodule update --init --recursive
fi

mkdir -p build && cd build
git log -1

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
    -DENABLE_SERVICE_SIMPLEFORWARDER=OFF
elif [ "$MODE" == "pcn-k8s" ]; then
  cmake .. -DENABLE_SERVICE_BRIDGE=OFF \
    -DENABLE_SERVICE_DDOSMITIGATOR=OFF \
    -DENABLE_SERVICE_FIREWALL=OFF \
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
    -DINSTALL_CLI=OFF
else
  cmake .. -DENABLE_PCN_IPTABLES=ON
fi
make -j $(getconf _NPROCESSORS_ONLN)
$SUDO make install

success_message
