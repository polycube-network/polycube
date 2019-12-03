Installing Polycube
===================

This installation guide includes instructions to install Polycube on ``Ubuntu 18.04``.
However those should also work on other versions and distributions with a few changes.

Dependencies
------------

Polycube requires following dependencies:
 - At least **Linux kernel v4.15** that includes a set of the eBPF features which are needed by polycube
 - **pistache**: a library to build rest API servers
 - **libints**: a library for crafting packets (needed only for some services)
 - **Go language**: required to run ``polycubectl`` (polycube command line interface)

Following sections will detail the installation process for the above components.

.. _updating-linux-kernel:

Updating Linux Kernel to v4.15
-------------------------------

Use ``uname -a`` to check the kernel version you are running, if this is ``<v4.15`` please use the following instructions to update it.

::

    wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.15/linux-headers-4.15.0-041500_4.15.0-041500.201802011154_all.deb
    wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.15/linux-headers-4.15.0-041500-generic_4.15.0-041500.201802011154_amd64.deb
    wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.15/linux-image-4.15.0-041500-generic_4.15.0-041500.201802011154_amd64.deb

    sudo dpkg -i *.deb
    sudo reboot

Automatic installation from source files
----------------------------------------

If you are running ``Ubuntu 18.04`` and you do not want to manually install polycube and its dependencies, you can use the install script available under the `scripts` folder.
This is not guaranteed that the script works on versions or distributions other than ``Ubuntu 18.04``.

Please notice that this script does not update the kernel version.

In this case you have to:

::

    # install git
    sudo apt-get install git

    # clone the polycube repository
    git clone https://github.com/polycube-network/polycube
    cd polycube
    git submodule update --init --recursive

    # launch the automatic install script (use -h to see the different installation modes)
    ./scripts/install.sh

Once the installation is completed, you can follow the :doc:`quickstart` instructions.

Note: if you have llvm 6.0 installed (check with ``apt list --installed | grep "llvm"``), the installation script will fail.
In this case, remove llvm 6.0 before starting the installation script:

::
    sudo apt remove llvm-6.0 llvm-6.0-dev llvm-6.0-runtime


Manual installation from source files
-------------------------------------

Install GO
^^^^^^^^^^

Go 1.8+ is needed to run ``polycubectl``, if you only plan to install ``polycubed`` you can skip this step.

::

    # If you are running a previous Ubuntu version, you could add the
    # longsleep/golang-backports ppa repository to get get required golang version.
    # sudo add-apt-repository ppa:longsleep/golang-backports
    # sudo apt-get update
    sudo apt-get install golang-go

    # Set $GOPATH, if not already set
    mkdir $HOME/go
    export GOPATH=$HOME/go

    # Check the Go version; you should get something
    # like 'go version go1.8.3 linux/amd64'
    go version

    # In order to make permanent the above changes, you can append export commands
    # to `~/.bashrc` or run the following commands and restart the terminal.
    echo 'export GOPATH=$HOME/go' >> ~/.bashrc


Install dependencies
^^^^^^^^^^^^^^^^^^^^

::

    # Install polycube dependencies
    sudo apt-get -y install git build-essential cmake bison flex \
        libelf-dev libllvm5.0 llvm-5.0-dev libclang-5.0-dev \
        libnl-route-3-dev libnl-genl-3-dev uuid-dev pkg-config \
        autoconf libtool m4 automake libssl-dev kmod jq bash-completion \
        gnupg2

Install libyang-dev
###################

::

    wget -nv http://download.opensuse.org/repositories/home:/liberouter/xUbuntu_18.04/amd64/libyang_0.14.81_amd64.deb -O libyang.deb
    wget -nv http://download.opensuse.org/repositories/home:/liberouter/xUbuntu_18.04/amd64/libyang-dev_0.14.81_amd64.deb -O libyang-dev.deb
    sudo apt install -f ./libyang.deb
    sudo apt install -y -f ./libyang-dev.deb
    rm ./libyang.deb
    rm ./libyang-dev.deb


If you are using another operating system please check the `libyang installation documentation <https://software.opensuse.org//download.html?project=home%3Aliberouter&package=libyang>`_.

Install pistache
################

::

    # Install Pistache (a library to create web servers that is used in polycubed)
    git clone https://github.com/oktal/pistache.git
    cd pistache
    # use last known working version
    git checkout 117db02eda9d63935193ad98be813987f6c32b33
    git submodule update --init
    mkdir build &&  cd build
    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DPISTACHE_USE_SSL=ON ..
    make -j $(getconf _NPROCESSORS_ONLN)
    sudo make install


Install libtins
###############

::

    # Install libtins (a library for network packet sniffing and crafting, used to create packets)
    git clone --branch v3.5 https://github.com/mfontanini/libtins.git
    cd libtins
    mkdir build && cd build
    cmake -DLIBTINS_ENABLE_CXX11=ON -DLIBTINS_BUILD_EXAMPLES=OFF \
        -DLIBTINS_BUILD_TESTS=OFF -DLIBTINS_ENABLE_DOT11=OFF \
        -DLIBTINS_ENABLE_PCAP=OFF -DLIBTINS_ENABLE_WPA2=OFF \
        -DLIBTINS_ENABLE_WPA2_CALLBACKS=OFF ..
    make -j $(getconf _NPROCESSORS_ONLN)
    sudo make install
    sudo ldconfig

Installing polycube
^^^^^^^^^^^^^^^^^^^

This installs the polycube daemon (``polycubed``), the polycube CLI (``polycubectl``) and the services shipped with polycube.
If you want to disable some services, you can modify the cmake flags using ``ccmake``.

::

    git clone https://github.com/polycube-network/polycube
    cd polycube
    git submodule update --init --recursive
    mkdir build && cd build
    # use 'ccmake ..' to change different compilation options as
    # enable/disable some services for example
    cmake ..
    make -j $(getconf _NPROCESSORS_ONLN)
    sudo make install


Hooray, you have ``polycube`` installed and ready to be used, please refer to :doc:`Quick Start <quickstart>` to start using your installation.
