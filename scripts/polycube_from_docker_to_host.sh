#!/bin/bash

#
# This script copies all the binaries, headers and libraries from a running Polycube docker image to the host's filesystem.
#
# By default the script assumes the running container name is 'polycubed'.
# It is possible to specify a different name as an argument
# e.g.: $ polycube_from_docker_to_host.sh myPolycubeContainer
#

image_name="polycubed"

if ! test -z "$1"
then
    image_name=$1
fi

# Copying binaries
sudo docker cp $image_name:/usr/local/bin /usr/local

# Copying headers
sudo mkdir -p /usr/include/polycube
sudo docker cp $image_name:/usr/include/polycube /usr/include

# Copying libraries
for lib in $(sudo docker exec $image_name ls /usr/lib | grep "libp")
do
    sudo docker cp $image_name:/usr/lib/$lib /usr/lib/
done
sudo ldconfig -n /usr/lib

sudo docker cp $image_name:/usr/local/lib /usr/local
sudo ldconfig -n /usr/local/lib

for lib in $(sudo docker exec $image_name ls /usr/lib/x86_64-linux-gnu/ | grep -P "libyang|libnl")
do
    sudo docker cp $image_name:/usr/lib/x86_64-linux-gnu/$lib /usr/lib/x86_64-linux-gnu
done

for lib in $(sudo docker exec $image_name ls /lib/x86_64-linux-gnu/ | grep -P "libnl")
do
    sudo docker cp $image_name:/lib/x86_64-linux-gnu/$lib /lib/x86_64-linux-gnu
done

# Copying base yang models
sudo mkdir -p /usr/local/include/polycube
sudo docker cp $image_name:/usr/local/include/polycube /usr/local/include/
sudo ldconfig -n /usr/local/include/

# Copying polycubectl
sudo docker cp $image_name:/usr/local/bin/polycubectl /usr/local/bin
