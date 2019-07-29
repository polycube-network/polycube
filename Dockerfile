# syntax = tonistiigi/dockerfile:runmount20180618 # enables --mount option for run
FROM ubuntu:18.04
ARG MODE=default
RUN --mount=target=/polycube cp -r /polycube /tmp/polycube && \
cd /tmp/polycube && \
SUDO="" USER="root" WORKDIR="/tmp/dev" ./scripts/install.sh $MODE && \
# install pcn-kubernetes only components
if [ "$MODE" = "pcn-k8s" ] ; then \
    cd /tmp && mkdir -p tmp && cd tmp && \
    curl -sS -L https://storage.googleapis.com/kubernetes-release/network-plugins/cni-0799f5732f2a11b329d9e3d51b9c8f2e3759f2ff.tar.gz -o cni.tar.gz && \
    tar -xvf cni.tar.gz && \
    mkdir /cni && \
    cp bin/loopback /cni && \
    cd /tmp/polycube/src/components/k8s/cni/polycube && \
    GOOS=linux go build -o /opt/cni/bin/polycube . && \
    cd /tmp/polycube/src/components/k8s/cni/conf && \
    GOOS=linux go build -o /cni-conf . && \
    cd /tmp/polycube/src/components/k8s/pcn_k8s/ && \
    GOOS=linux go build -o /pcn_k8s . ; \
fi && \
apt-get purge --auto-remove -y git bison cmake flex \
libllvm5.0 llvm-5.0-dev libclang-5.0-dev uuid-dev autoconf software-properties-common golang-go \
libtool curl && \
apt-get clean && \
rm -fr /root /var/lib/apt/lists/* /tmp/* /var/tmp/* /usr/local/go/*
# TODO: which other apt-get packages can be removed?

ADD src/components/k8s/cni/cni-install.sh /cni-install.sh
ADD src/components/k8s/cni/cni-uninstall.sh /cni-uninstall.sh
ADD src/components/k8s/pcn_k8s/cleanup.sh /cleanup.sh
ADD src/components/k8s/pcn_k8s/init.sh /init.sh

CMD ["polycubed"]
