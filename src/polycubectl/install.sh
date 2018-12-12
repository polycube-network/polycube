#!/bin/bash

# please note this is a testing script to install polycubectl
# when repository will be public, a simple go get could solve the problem

function error_message {
  set +x
  echo
  echo 'Error during installation'
  exit 1
}
function success_message {
  set +x
  echo
  echo 'Installation completed successfully'
  exit 0
}
trap error_message ERR

[[ -z "${USER}" ]] && USER='root'

if [[ -z "${GOPATH}" ]]; then
  echo
  echo "GOPATH env var is not set. Please set it by typing:"
  echo "export GOPATH=\$HOME/go"
  echo "#and type again:"
  echo "cmake .."

  error_message
fi

# Print commands and their arguments as they are executed
set -x
# Exit immediately if a command exits with a non-zero status.
set -e

echo "installing polycubectl ..."

pwdir=$(pwd)
mkdir -p $GOPATH/src/github.com/polycube-network/polycube/src/

# create soft link inside go directory
ln -f -s $pwdir $GOPATH/src/github.com/polycube-network/polycube/src/ > /dev/null 2>&1

cd $GOPATH/src/github.com/polycube-network/polycube/src/polycubectl
# get all go dependencies
go get ./...

go build -o /usr/local/bin/polycubectl

set +e
# This command forces CLI config file to be removed and regenerated with default values each time
# we run 'sudo make install'
rm -f /home/$USER/.config/polycube/polycubectl_config.yaml

success_message

echo "polycubectl ? to show commands"
