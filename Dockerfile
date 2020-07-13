# syntax = tonistiigi/dockerfile:runmount20180618
###############################################
# STEP 1: using an Ubuntu 18.04 image to install the entire Polycube framework with all dependencies.
# 
# During this step an ubuntu image is used to compile Polycube with all its dependencies.
# This dockerfile is reused for all the DEFAULT_MODE (default, pcn-iptables, pcn-k8s) which, thanks to install.sh and pre-requirements.sh scripts,
# enable/disable Polycube services and install specific tools.
FROM ubuntu:18.04 AS builder

ARG DEFAULT_MODE=default
ENV MODE=$DEFAULT_MODE
RUN --mount=target=/polycube cp -r /polycube /tmp/polycube && cd /tmp/polycube && \
mkdir -p /usr/local/share/polycube /usr/local/include/polycube /opt/cni/bin /cni && touch /cni-conf /pcn_k8s /opt/cni/bin/polycube && \
SUDO="" USER="root" WORKDIR="/tmp/dev" ./scripts/install.sh $MODE && \
# install pcn-k8s only components
if [ "$MODE" = "pcn-k8s" ] ; then \
    cd /tmp && mkdir -p tmp && cd tmp && \
    curl -sS -L https://storage.googleapis.com/kubernetes-release/network-plugins/cni-0799f5732f2a11b329d9e3d51b9c8f2e3759f2ff.tar.gz -o cni.tar.gz && \
    tar -xvf cni.tar.gz && \
    cp bin/loopback /cni && \
    cd /tmp/polycube/src/components/k8s/cni/polycube && \
    GOOS=linux go build -o /opt/cni/bin/polycube . && \
    cd /tmp/polycube/src/components/k8s/cni/conf && \
    GOOS=linux go build -o /cni-conf . && \
    cd /tmp/polycube/src/components/k8s/pcn_k8s/ && \
    GOOS=linux go build -o /pcn_k8s . ; \
fi

# copying cni scripts (will be removed if not needed)
ADD src/components/k8s/cni/cni-install.sh src/components/k8s/cni/cni-uninstall.sh src/components/k8s/pcn_k8s/cleanup.sh src/components/k8s/pcn_k8s/init.sh /

###############################################
# STEP 2: using an Ubuntu 18.04 image and extracting executables obtained by the image created at STEP 1.
# 
# During this final step a new fresh ubuntu image is used and all the previously generated executables/libraries are copied.
# This way, this final image contains only the result of the compilation and NOT the entire softwares/scripts needed at compile-time.
# Moreover, depending on the DEFAULT_MODE, some tools that are needed at runtime (like iptables and iproute2 for the DEFAULT_MODE=pcn-k8s) are
# installed.

FROM ubuntu:18.04

ARG DEFAULT_MODE=default
ENV MODE=$DEFAULT_MODE
# copying binaries
COPY --from=builder /usr/local/bin /usr/local/bin
COPY --from=builder /usr/local/share/polycube /usr/local/share/polycube
# copying polycube services
COPY --from=builder /usr/lib/lib*.so /usr/lib/
# copying libpistache libyang libtins libprometheus .so
COPY --from=builder /usr/local/lib/lib*.so /usr/local/lib/
# copying libyang folder containing plugins (needed for yanglint)
COPY --from=builder /usr/local/lib/libyang /usr/local/lib/libyang 
# copying main OS libraries
COPY --from=builder /usr/lib/x86_64-linux-gnu/libnl*.so /usr/lib/x86_64-linux-gnu/libcrypto*.so\
	/usr/lib/x86_64-linux-gnu/libelf*.so /usr/lib/x86_64-linux-gnu/libssl*.so \
	/usr/lib/x86_64-linux-gnu/libpcap*.so /usr/lib/x86_64-linux-gnu/
COPY --from=builder /lib/x86_64-linux-gnu/libnl*.so /lib/x86_64-linux-gnu/
# copying base yang model
COPY --from=builder /usr/local/include/polycube /usr/local/include/polycube
# copying k8s scripts and cni if present
COPY --from=builder /*.sh /cni* /pcn_k8s /
COPY --from=builder /opt/cni/bin/polycube /opt/cni/bin/
# creating log dir and file + configuring links for libraries + installing essential runtime tools
RUN mkdir -p /var/log/polycube && touch /var/log/polycube/polycubed.log && \
# install pcn-k8s only essential tools, else remove useless scripts
if [ "$MODE" = "pcn-k8s" ] ; then \
    apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -yq iptables iproute2 && \
    apt-get purge --auto-remove && apt-get clean ; \
else \
    rm -rf /opt/cni/bin/polycube /*.sh /pcn_k8s /cni*; \
fi && ldconfig

# by running nsenter --mount=/host/proc/1/ns/mnt polycubed, the daemon has a complete view of the namespaces of the host and it is able to manipulate them (needed for shadow services)
CMD ["nsenter","--mount=/host/proc/1/ns/mnt","polycubed"]
