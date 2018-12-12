#!/bin/bash

function error_message {
  set +x
  echo
  echo 'Error during installation'
}

function success_message {
  set +x
  echo
  echo 'Installation completed successfully'
}
trap error_message ERR

set -x
set -e
set -u

HOST_PREFIX=${HOST_PREFIX:-/host}
CNI_CONF_NAME=${CNI_CONF_NAME:-10-polycube-cni.conf}

CNI_DIR=${CNI_DIR:-${HOST_PREFIX}/opt/cni}
POLYCUBE_CNI_CONF=${POLYCUBE_CNI_CONF:-${HOST_PREFIX}/etc/cni/net.d/${CNI_CONF_NAME}}

# Install the CNI loopback driver if not installed already
if [ ! -f ${CNI_DIR}/bin/loopback ]; then
	echo "Installing loopback driver..."
	cp /cni/loopback ${CNI_DIR}/bin/
fi

cp /opt/cni/bin/polycube ${CNI_DIR}/bin/
# creates polycube-cni configuration file
/cni-conf ${POLYCUBE_CNI_CONF}
