#!/bin/bash
if [ -z "$DIR" ]
then
    DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
fi
mkdir -p $WORKDIR

$SUDO sudo apt update
$SUDO bash -c "apt install --allow-unauthenticated -y wget gnupg2 software-properties-common"

# golang v1.12 still not available in repos
$SUDO add-apt-repository ppa:longsleep/golang-backports -y || true

# repo for libyang-dev

# Cannot add the repository to our machine, as the GPG key expired on Aug 2019 and Ubuntu refuses to install the software in there
#$SUDO sh -c "echo 'deb http://download.opensuse.org/repositories/home:/liberouter/xUbuntu_18.04/ /' > /etc/apt/sources.list.d/home:liberouter.list"
#wget -nv https://download.opensuse.org/repositories/home:liberouter/xUbuntu_18.04/Release.key -O Release.key
#$SUDO apt-key add - < Release.key
# So, installing the required package by downloading it manually
wget -nv http://download.opensuse.org/repositories/home:/liberouter/xUbuntu_18.04/amd64/libyang_0.14.81_amd64.deb -O libyang.deb
wget -nv http://download.opensuse.org/repositories/home:/liberouter/xUbuntu_18.04/amd64/libyang-dev_0.14.81_amd64.deb -O libyang-dev.deb
$SUDO apt install -f ./libyang.deb
$SUDO apt install -y -f ./libyang-dev.deb
rm ./libyang.deb
rm ./libyang-dev.deb

$SUDO apt update


PACKAGES=""
PACKAGES+=" git" # needed to clone dependencies
PACKAGES+=" build-essential cmake" # provides compiler and other compilation tools
PACKAGES+=" bison flex libelf-dev" # bcc dependencies
PACKAGES+=" libllvm5.0 llvm-5.0-dev libclang-5.0-dev" # bpf tools compilation toolchain
PACKAGES+=" libnl-route-3-dev libnl-genl-3-dev" # netlink library
PACKAGES+=" uuid-dev"
PACKAGES+=" golang-go" # needed for polycubectl and pcn-k8s
PACKAGES+=" pkg-config"
# Removed because of the comment at line L76 (GPG key expired); we had to install this manually
#PACKAGES+=" libyang-dev"
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

# use non interactive to avoid blocking the install script
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
