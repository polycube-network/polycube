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

set -x
# Exit immediately if a command exits with a non-zero status.
set -e

echo "installing polycubectl ..."
if ! command -v go &> /dev/null
then
    echo "go command not found"
    echo "hint: go may not be available for root user"
    exit 1
fi

# get all go dependencies
go get ./...

go build -o /usr/local/bin/polycubectl

set +e
# This command forces CLI config file to be removed and regenerated with default values each time
# we run 'sudo make install'
rm -f /home/$USER/.config/polycube/polycubectl_config.yaml

success_message

echo "polycubectl ? to show commands"
