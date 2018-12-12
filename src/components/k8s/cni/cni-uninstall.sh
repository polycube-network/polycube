#!/bin/bash

function error_message {
  set +x
  echo
  echo 'Error during uninstallation'
}

function success_message {
  set +x
  echo
  echo 'uninstallation completed successfully'
}
trap error_message ERR

set -x
set -e
set -u

HOST_PREFIX=${HOST_PREFIX:-/host}
CNI_CONF_NAME=${CNI_CONF_NAME:-10-polycube-cni.conf}

CNI_DIR=${CNI_DIR:-${HOST_PREFIX}/opt/cni}
POLYCUBE_CNI_CONF=${POLYCUBE_CNI_CONF:-${HOST_PREFIX}/etc/cni/net.d/${CNI_CONF_NAME}}

rm -rf /opt/cni/bin/polycube
rm -rf ${POLYCUBE_CNI_CONF}
