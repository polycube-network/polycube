#!/bin/bash

export GO15VENDOREXPERIMENT=1

function error_message {
  set +x
  echo
  echo 'Error during CNI building'
  exit 1
}

function success_message {
  set +x
  echo
  echo 'CNI plugin successfully built'
  exit 0
}
trap error_message ERR

set -x
set -e

echo "Building CNI plugin..."

CNI_CONF_NAME=${CNI_CONF_NAME:-10-polycube-cni.conf}

pwdir=$(pwd)

if [ ! -f $GOPATH/src/github.com/polycube-network/polycube ]; then
	echo "mkdir"
	mkdir -p $GOPATH/src/github.com/polycube-network/polycube/src/components
  path=${pwdir%/cni}
	# create soft link inside go directory
  ln -f -s $path $GOPATH/src/github.com/polycube-network/polycube/src/components/ > /dev/null 2>&1
fi

# compile and copy polycube to the right folder
cd $GOPATH/src/github.com/polycube-network/polycube/src/components/k8s/cni/polycube
go get github.com/sirupsen/logrus
go install
mkdir -p /opt/cni/bin
cp $GOPATH/bin/polycube /opt/cni/bin
mkdir -p /etc/cni/net.d

success_message
