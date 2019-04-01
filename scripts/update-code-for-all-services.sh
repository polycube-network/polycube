#!/bin/bash

# This script updates the code for all the existing services.
# Files present in swagger-codegen-ignore are not updated.

if [ -z ${POLYCUBE_HOME+x} ]; then
    echo "\$POLYCUBE_HOME is not set"
    exit 1
fi

SERVICES=(#bridge
          ddosmitigator
          firewall
          helloworld
          iptables
          k8sfilter
          k8switch
          #loadbalancer-dsr
          #loadbalancer-rp
          nat
          pbforwarder
          router
          simplebridge
          simpleforwarder
          transparent-helloworld)

for SERVICE in "${SERVICES[@]}"
do
    echo $SERVICE
    ./polycube-codegen.sh -i $POLYCUBE_HOME/src/services/pcn-$SERVICE/datamodel/$SERVICE.yang \
        -o $POLYCUBE_HOME/src/services/pcn-$SERVICE
done

# lbrp and lbdsr are a little bit special

SERVICE=lbrp
SERVICE_PATH=loadbalancer-rp
echo $SERVICE
./polycube-codegen.sh -i $POLYCUBE_HOME/src/services/pcn-$SERVICE_PATH/datamodel/$SERVICE.yang \
    -o $POLYCUBE_HOME/src/services/pcn-$SERVICE_PATH

SERVICE=lbdsr
SERVICE_PATH=loadbalancer-dsr
echo $SERVICE
./polycube-codegen.sh -i $POLYCUBE_HOME/src/services/pcn-$SERVICE_PATH/datamodel/$SERVICE.yang \
    -o $POLYCUBE_HOME/src/services/pcn-$SERVICE_PATH
