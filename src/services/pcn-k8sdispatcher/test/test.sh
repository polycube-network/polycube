#!/bin/bash

function cleanup {
  set +e
  polycubectl k8sdispatcher del k0
}
trap cleanup EXIT

set -x
set -e

polycubectl k8sdispatcher add k0 internal-src-ip=3.3.1.1 loglevel=TRACE
polycubectl k0 show
polycubectl k0 ports show
polycubectl k0 nodeport-rule show
polycubectl k0 session-rule show
polycubectl k0 ports add to_frontend type=FRONTEND ip=10.10.10.10
polycubectl k0 show ports to_frontend
polycubectl k0 ports add to_backend type=BACKEND
polycubectl k0 show ports to_backend
polycubectl k0 nodeport-rule add 32000 TCP external-traffic-policy=LOCAL rule-name=my-rule
polycubectl k0 show nodeport-rule 32000 TCP
polycubectl k0 set nodeport-range=32000-32500
polycubectl k0 show nodeport-range
