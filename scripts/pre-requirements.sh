#!/bin/bash
if [ -z "$DIR" ]
then
    DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
fi
mkdir -p $WORKDIR

$SUDO apt update
$SUDO bash -c "apt install --allow-unauthenticated -y wget gnupg2 software-properties-common"

# Release in which has been natively introduced support for golang-go (Ubuntu >= 20.04)
os_limit_major="20"
os_limit_minor="04"
read -r os_major os_minor<<<$(grep -Po 'VERSION_ID="\K.*?(?=")' /etc/os-release | sed 's/\./ /g')



$SUDO apt update
PACKAGES=""
PACKAGES+=" git" # needed to clone dependencies
PACKAGES+=" build-essential cmake" # provides compiler and other compilation tools
PACKAGES+=" bison flex libelf-dev" # bcc dependencies
PACKAGES+=" libllvm-9-ocaml-dev libllvm9 llvm-9 llvm-9-dev llvm-9-doc llvm-9-examples llvm-9-runtime clang-9 lldb-9 lld-9 llvm-9-tools libclang-9-dev"
PACKAGES+=" libllvm9 llvm-9-dev libclang-9-dev" # bpf tools compilation tool chain
PACKAGES+=" libnl-route-3-dev libnl-genl-3-dev" # netlink library
PACKAGES+=" uuid-dev"
PACKAGES+=" pkg-config"
# Removed because of the comment at line L76 (GPG key expired); we had to install this manually
#PACKAGES+=" libyang-dev"
PACKAGES+=" autoconf libtool m4 automake"
PACKAGES+=" libssl-dev" # needed for certificate based security
PACKAGES+=" sudo" # needed for pcn-iptables, when building docker image
PACKAGES+=" kmod" # needed for pcn-iptables, when using lsmod to unload conntrack if not needed
PACKAGES+=" jq bash-completion" # needed for polycubectl bash autocompletion
PACKAGES+=" libpcre3-dev" # needed for libyang
PACKAGES+=" libpcap-dev" # needed for packetcapture filter

  

if ! command -v go &> /dev/null
then
   # Checking whether the major release is lower or the minor
  if  (( os_major < os_limit_major || ( os_major == os_limit_major && os_minor < os_limit_minor ) ))
  then
    PACKAGES+=" golang-go" # needed for polycubectl and pcn-k8s
    $SUDO add-apt-repository ppa:longsleep/golang-backports -y || true
  fi
fi

if [ "$MODE" == "pcn-k8s" ]; then
  PACKAGES+=" curl" # needed for pcn-k8s to download a binary
  PACKAGES+=" iptables" # only for pcn-k8s
  PACKAGES+=" iproute2" # provides bridge command that is used to add entries in vxlan device
fi

# use non interactive to avoid blocking the install script
$SUDO bash -c "DEBIAN_FRONTEND=noninteractive apt-get install -yq $PACKAGES"

# licd $WORKDIR
set +e
if [ ! -d libyang ]; then
    git clone https://github.com/CESNET/libyang.git
fi
cd libyang
git checkout v0.14-r1
mkdir -p build && cd build
cmake ..
make -j $(getconf _NPROCESSORS_ONLN)
$SUDO make install

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
