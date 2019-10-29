#!/bin/bash
container="polycubed"
if ! test -z "$1"
then
    container=$1
fi
# Checking if the container is up
docker ps
# Copying binaries
echo "# Copying binaries"
echo "container content in /usr/local/bin:"
docker exec $container /bin/ls /usr/local/bin
echo "Copying $container:/usr/local/bin to /usr/local"
sudo docker cp -a $container:/usr/local/bin /usr/local
echo "container content in /usr/local/share/polycube:"
docker exec $container /bin/ls /usr/local/share/polycube
echo "Copying $container:/usr/local/share/polycube to /usr/local/share"
sudo docker cp -a $container:/usr/local/share/polycube /usr/local/share
# Copying headers
echo "# Copying headers"
echo "container content in /usr/include/polycube:"
docker exec $container /bin/ls /usr/include/polycube
echo "Copying $container:/usr/include/polycube to /usr/include"
sudo mkdir -p /usr/include/polycube
sudo docker cp -a $container:/usr/include/polycube /usr/include
# Copying libraries
echo "# Copying libraries"
echo "container content in /usr/lib:"
docker exec $container /bin/ls /usr/lib | /bin/grep "libp"
for lib in $(docker exec $container ls /usr/lib | grep "libp"); do
    echo "Copying $container:/usr/lib/$lib to /usr/lib/"
    sudo docker cp $container:/usr/lib/$lib /usr/lib/
done
echo "container content in /usr/local/lib:"
docker exec $container /bin/ls /usr/local/lib | /bin/grep "lib"
for lib in $(docker exec $container ls /usr/local/lib | grep "lib"); do
    echo "Copying $container:/usr/local/lib/$lib to /usr/local/lib"
    sudo docker cp $container:/usr/local/lib/$lib /usr/local/lib
done
echo "container content in /usr/lib/x86_64-linux-gnu:"
docker exec $container /bin/ls /usr/lib/x86_64-linux-gnu | /bin/grep -P "libyang|libnl"
for lib in $(docker exec $container ls /usr/lib/x86_64-linux-gnu/ | grep -P "libyang|libnl"); do
    echo "Copying $container:/usr/lib/x86_64-linux-gnu/$lib to /usr/lib/x86_64-linux-gnu"
    sudo docker cp $container:/usr/lib/x86_64-linux-gnu/$lib /usr/lib/x86_64-linux-gnu
done
echo "container content in /lib/x86_64-linux-gnu:"
docker exec $container /bin/ls /lib/x86_64-linux-gnu | /bin/grep -P "libnl"
for lib in $(docker exec $container ls /lib/x86_64-linux-gnu | grep -P "libnl"); do
    echo "Copying $container:/lib/x86_64-linux-gnu/$lib to /lib/x86_64-linux-gnu"
    sudo docker cp $container:/lib/x86_64-linux-gnu/$lib /lib/x86_64-linux-gnu
done
# Copying base yang models
echo "# Copying base yang models"
echo "container content in /usr/local/include/polycube:"
docker exec $container /bin/ls /usr/local/include/polycube
sudo mkdir -p /usr/local/include/polycube
echo "Copying $container:/usr/local/include/polycube to /usr/local/include/"
sudo docker cp -a $container:/usr/local/include/polycube /usr/local/include/
sudo ldconfig