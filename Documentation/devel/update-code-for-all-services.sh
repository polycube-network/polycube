#!/bin/bash

# This script updates the code for all the existing services.
# Files present in swagger-codegen-ignore are not updated.

SERVICES=(#bridge
          ddosmitigator
          firewall
          helloworld
          k8sfilter
          k8switch
          #loadbalancer-dsr
          #loadbalancer-rp
          nat
          pbforwarder
          router
          simplebridge
          simpleforwarder)

for SERVICE in "${SERVICES[@]}"
do
    echo $SERVICE
    ./polycube-codegen.sh -i $POLYCUBE_HOME/src/services/pcn-$SERVICE/resources/$SERVICE.yang \
        -o $POLYCUBE_HOME/src/services/pcn-$SERVICE
done

# lbrp and lbdsr are a little bit special

SERVICE=lbrp
SERVICE_PATH=loadbalancer-rp
echo $SERVICE
./polycube-codegen.sh -i $POLYCUBE_HOME/src/services/pcn-$SERVICE_PATH/resources/$SERVICE.yang \
    -o $POLYCUBE_HOME/src/services/pcn-$SERVICE_PATH

SERVICE=lbdsr
SERVICE_PATH=loadbalancer-dsr
echo $SERVICE
./polycube-codegen.sh -i $POLYCUBE_HOME/src/services/pcn-$SERVICE_PATH/resources/$SERVICE.yang \
    -o $POLYCUBE_HOME/src/services/pcn-$SERVICE_PATH
