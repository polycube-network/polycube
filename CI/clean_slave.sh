#!/bin/bash
# Removing binaries
echo "# Removing binaries"
echo "removing /usr/local/bin/pcn*"
sudo rm -rf /usr/local/bin/pcn*
echo "removing /usr/local/bin/polycube*"
sudo rm -rf /usr/local/bin/polycube*
echo "removing /usr/local/share/polycube"
sudo rm -rf /usr/local/share/polycube
# Removing headers
echo "# Removing headers"
echo "removing /usr/include/polycube"
sudo rm -rf /usr/include/polycube
# Removing libraries
echo "# Removing libraries"
for lib in $(ls /usr/lib | grep "libp"); do
    echo "removing /usr/lib/$lib"
    sudo rm -rf /usr/lib/$lib
done
for lib in $(ls /usr/local/lib | grep "lib"); do
    echo "removing /usr/local/lib/$lib"
    sudo rm -rf /usr/local/lib/$lib
done
for lib in $(ls /usr/lib/x86_64-linux-gnu/ | grep -P "libyang|libnl"); do
    echo "removing /usr/lib/x86_64-linux-gnu/$lib"
    sudo rm -rf /usr/lib/x86_64-linux-gnu/$lib
done
for lib in $(ls /lib/x86_64-linux-gnu/ | grep -P "libnl"); do
    echo "removing /lib/x86_64-linux-gnu/$lib"
    sudo rm -rf /lib/x86_64-linux-gnu/$lib
done
sudo ldconfig