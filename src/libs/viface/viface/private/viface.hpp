/**
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef _VIFACE_PRIV_HPP
#define _VIFACE_PRIV_HPP

// Standard
#include <stdexcept>   // Exceptions
#include <sstream>     // ostringstream
#include <fstream>     // ifstream
#include <iomanip>     // setw
#include <map>         // map

// C
#include <cstring>     // memset
#include <cerrno>      // EAGAIN

// Posix
#include <unistd.h>    // open(), close()
#include <fcntl.h>     // O_RDWR
#include <dirent.h>    // opendir, readdir, closedir

// Network
#include <arpa/inet.h> // inet_ntop()

// Linux TUN/TAP includes
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/if_arp.h>

// Interfaces
#include <ifaddrs.h>   // getifaddrs

// Framework
#include "viface/viface.hpp"

using namespace std;
using namespace viface;

namespace viface
{
struct in6_ifreq
{
    struct in6_addr ifr6_addr;
    uint32_t ifr6_prefixlen;
    int ifr6_ifindex;
};

struct viface_queues
{
    int rx;
    int tx;
};

class VIfaceImpl
{
    private:

        struct viface_queues queues;
        int kernel_socket;
        int kernel_socket_ipv6;
        vector<uint8_t> pktbuff;

        string name;
        uint id;
        string mac;
        string ipv4;
        string netmask;
        string broadcast;
        set<string> ipv6s;
        uint mtu;

        set<string> stats_keys_cache;
        map<string,uint64_t> stats_cache;

        static uint idseq;

    public:

        VIfaceImpl(string name, bool tap, int id);
        ~VIfaceImpl();

        string getName() const
        {
            return this->name;
        }

        uint getID() const
        {
            return this->id;
        }

        int getTX() const
        {
            return this->queues.tx;
        }

        int getRX() const
        {
            return this->queues.rx;
        }

        void setMAC(string mac);

        string getMAC() const;

        string ioctlGetIPv4(unsigned long request) const;

        void setIPv4(string ipv4);

        string getIPv4() const;

        void setIPv4Netmask(string netmask);

        string getIPv4Netmask() const;

        void setIPv4Broadcast(string broadcast);

        string getIPv4Broadcast() const;

        void setIPv6(set<string> const& ipv6s);

        set<string> getIPv6() const;

        void setMTU(uint mtu);

        uint getMTU() const;

        void up();

        void down() const;

        bool isUp() const;

        vector<uint8_t> receive();

        void send(vector<uint8_t>& packet) const;

        set<string> listStats();

        uint64_t readStatFile(string const& stat);

        uint64_t readStat(string const& stat);

        void clearStat(string const& stat);
};
};
#endif // _VIFACE_PRIV_HPP
