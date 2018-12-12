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

#include "viface/private/viface.hpp"

namespace viface
{
/*= Utilities ================================================================*/

namespace utils
{
vector<uint8_t> parse_mac(string const& mac)
{
    unsigned int bytes[6];
    int scans = sscanf(
        mac.c_str(),
        "%02x:%02x:%02x:%02x:%02x:%02x",
        &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4], &bytes[5]
        );

    if (scans != 6) {
        ostringstream what;
        what << "--- Invalid MAC address " << mac << endl;
        throw invalid_argument(what.str());
    }

    vector<uint8_t> parsed(6);
    parsed.assign(&bytes[0], &bytes[6]);
    return parsed;
}

string hexdump(vector<uint8_t> const& bytes)
{
    ostringstream buff;

    int buflen = bytes.size();

    int i;
    int j;
    char c;

    for (i = 0, j = 0; i < buflen; i += 16) {
        // Print offset
        buff << right << setfill('0') << setw(4) << i << "  ";

        // Print bytes in hexadecimal
        buff << hex;
        for (j = 0; j < 16; j++) {
            if (i + j < buflen) {
                c = bytes[i + j];
                buff << setfill('0') << setw(2) << (int(c) & 0xFF);
                buff << " ";
            } else {
                buff << "   ";
            }
        }
        buff << dec;
        buff << " ";

        // Print printable characters
        for (j = 0; j < 16; j++) {
            if (i + j < buflen) {
                c = bytes[i + j];
                if (isprint(c)) {
                    buff << c;
                } else {
                    buff << ".";
                }
            }
        }
        buff << endl;
    }
    return buff.str();
}

uint32_t crc32(vector<uint8_t> const& bytes)
{
    static uint32_t crc_table[] = {
        0x4DBDF21C, 0x500AE278, 0x76D3D2D4, 0x6B64C2B0,
        0x3B61B38C, 0x26D6A3E8, 0x000F9344, 0x1DB88320,
        0xA005713C, 0xBDB26158, 0x9B6B51F4, 0x86DC4190,
        0xD6D930AC, 0xCB6E20C8, 0xEDB71064, 0xF0000000
    };

    uint32_t crc = 0;
    const uint8_t* data = &bytes[0];

    for (uint32_t i = 0; i < bytes.size(); ++i) {
        crc = (crc >> 4) ^ crc_table[(crc ^ data[i]) & 0x0F];
        crc = (crc >> 4) ^ crc_table[(crc ^ (data[i] >> 4)) & 0x0F];
    }

    return crc;
}
}


/*= Helpers ==================================================================*/

static void read_flags(int sockfd, string name, struct ifreq& ifr)
{
    ostringstream what;

    // Prepare communication structure
    memset(&ifr, 0, sizeof(struct ifreq));

    // Set interface name
    (void) strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);

    // Read interface flags
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) != 0) {
        what << "--- Unable to read " << name << " flags." << endl;
        what << "    Error: " << strerror(errno);
        what << " (" << errno << ")." << endl;
        throw runtime_error(what.str());
    }
}

static uint read_mtu(string name, size_t size_bytes)
{
    int fd = -1;
    int nread = -1;
    ostringstream what;
    char buffer[size_bytes + 1];

    // Opens MTU file
    fd = open(("/sys/class/net/" + name + "/mtu").c_str(),
              O_RDONLY | O_NONBLOCK);

    if (fd < 0) {
        what << "--- Unable to open MTU file for '";
        what << name << "' network interface." << endl;
        what << "    Error: " << strerror(errno);
        what << " (" << errno << ")." << endl;
        goto err;
    }

    // Reads MTU value
    nread = read(fd, &buffer, size_bytes);
    buffer[size_bytes] = '\0';

    // Handles errors
    if (nread == -1) {
        what << "--- Unable to read MTU for '";
        what << name << "' network interface." << endl;
        what << "    Error: " << strerror(errno);
        what << " (" << errno << ")." << endl;
        goto err;
    }

    if (close(fd) < 0) {
        what << "--- Unable to close MTU file for '";
        what << name << "' network interface." << endl;
        what << "    Error: " << strerror(errno);
        what << " (" << errno << ")." << endl;
        goto err;
    }

    return stoul(buffer, nullptr, 10);

err:
    // Rollback close file descriptor
    close(fd);

    throw runtime_error(what.str());
}

static string alloc_viface(string name, bool tap, struct viface_queues* queues)
{
    int i = 0;
    int fd = -1;
    ostringstream what;

    /* Create structure for ioctl call
     *
     * Flags: IFF_TAP   - TAP device (layer 2, ethernet frame)
     *        IFF_TUN   - TUN device (layer 3, IP packet)
     *        IFF_NO_PI - Do not provide packet information
     *        IFF_MULTI_QUEUE - Create a queue of multiqueue device
     */
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_NO_PI | IFF_MULTI_QUEUE;
    if (tap) {
        ifr.ifr_flags |= IFF_TAP;
    } else {
        ifr.ifr_flags |= IFF_TUN;
    }

    (void) strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);

    // Allocate queues
    for (i = 0; i < 2; i++) {
        // Open TUN/TAP device
        fd = open("/dev/net/tun", O_RDWR | O_NONBLOCK);
        if (fd < 0) {
            what << "--- Unable to open TUN/TAP device." << endl;
            what << "    Name: " << name << " Queue: " << i << endl;
            what << "    Error: " << strerror(errno);
            what << " (" << errno << ")." << endl;
            goto err;
        }

        // Register a network device with the kernel
        if (ioctl(fd, TUNSETIFF, (void *)&ifr) != 0) {
            what << "--- Unable to register a TUN/TAP device." << endl;
            what << "    Name: " << name << " Queue: " << i << endl;
            what << "    Error: " << strerror(errno);
            what << " (" << errno << ")." << endl;

            if (close(fd) < 0) {
                what << "--- Unable to close a TUN/TAP device." << endl;
                what << "    Name: " << name << " Queue: " << i << endl;
                what << "    Error: " << strerror(errno);
                what << " (" << errno << ")." << endl;
            }
            goto err;
        }

        ((int *)queues)[i] = fd;
    }

    return string(ifr.ifr_name);

err:
    // Rollback close file descriptors
    for (--i; i >= 0; i--) {
        if (close(((int *)queues)[i]) < 0) {
            what << "--- Unable to close a TUN/TAP device." << endl;
            what << "    Name: " << name << " Queue: " << i << endl;
            what << "    Error: " << strerror(errno);
            what << " (" << errno << ")." << endl;
        }
    }

    throw runtime_error(what.str());
}

static void hook_viface(string name, struct viface_queues* queues)
{
    int i = 0;
    int fd = -1;
    ostringstream what;

    // Creates Tx/Rx sockets and allocates queues
    for (i = 0; i < 2; i++) {
        // Creates the socket
        fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

        if (fd < 0) {
            what << "--- Unable to create the Tx/Rx socket channel." << endl;
            what << "    Name: " << name << " Queue: " << i << endl;
            what << "    Error: " << strerror(errno);
            what << " (" << errno << ")." << endl;
            goto err;
        }

        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));

        (void) strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);

        // Obtains the network index number
        if (ioctl(fd, SIOCGIFINDEX, &ifr) != 0) {
            what << "--- Unable to get network index number.";
            what << "    Name: " << name << " Queue: " << i << endl;
            what << "    Error: " << strerror(errno);
            what << " (" << errno << ")." << endl;
            goto err;
        }

        struct sockaddr_ll socket_addr;
        memset(&socket_addr, 0, sizeof(struct sockaddr_ll));

        socket_addr.sll_family = AF_PACKET;
        socket_addr.sll_protocol = htons(ETH_P_ALL);
        socket_addr.sll_ifindex = ifr.ifr_ifindex;

        // Binds the socket to the 'socket_addr' address
        if (bind(fd, (struct sockaddr*) &socket_addr,
                 sizeof(socket_addr)) != 0) {
            what << "--- Unable to bind the Tx/Rx socket channel to the '";
            what << name << "' network interface." << endl;
            what << "    Queue: " << i << "." << endl;
            what << "    Error: " << strerror(errno);
            what << " (" << errno << ")." << endl;
            goto err;
        }

        ((int *)queues)[i] = fd;
    }

    return;

err:
    // Rollback close file descriptors
    for (--i; i >= 0; i--) {
        if (close(((int *)queues)[i]) < 0) {
            what << "--- Unable to close a Rx/Tx socket." << endl;
            what << "    Name: " << name << " Queue: " << i << endl;
            what << "    Error: " << strerror(errno);
            what << " (" << errno << ")." << endl;
        }
    }

    throw runtime_error(what.str());
}


/*= Virtual Interface Implementation =========================================*/

uint VIfaceImpl::idseq = 0;

VIfaceImpl::VIfaceImpl(string name, bool tap, int id)
{
    // Check name length
    if (name.length() >= IFNAMSIZ) {
        throw invalid_argument("--- Virtual interface name too long.");
    }

    // Create queues
    struct viface_queues queues;
    memset(&queues, 0, sizeof(struct viface_queues));

    /* Checks if the path name can be accessed. If so,
     * it means that the network interface is already defined.
     */
    if (access(("/sys/class/net/" + name).c_str(), F_OK) == 0) {
        hook_viface(name, &queues);
        this->name = name;

        // Read MTU value and resize buffer
        this->mtu = read_mtu(name, sizeof(this->mtu));
        this->pktbuff.resize(this->mtu);
    } else {
        this->name = alloc_viface(name, tap, &queues);

        // Other defaults
        this->mtu = 1500;
    }

    this->queues = queues;

    // Create socket channels to the NET kernel for later ioctl
    this->kernel_socket = -1;
    this->kernel_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (this->kernel_socket < 0) {
        ostringstream what;
        what << "--- Unable to create IPv4 socket channel to the NET kernel.";
        what << endl;
        what << "    Error: " << strerror(errno);
        what << " (" << errno << ")." << endl;
        throw runtime_error(what.str());
    }

    this->kernel_socket_ipv6 = -1;
    this->kernel_socket_ipv6 = socket(AF_INET6, SOCK_STREAM, 0);
    if (this->kernel_socket_ipv6 < 0) {
        ostringstream what;
        what << "--- Unable to create IPv6 socket channel to the NET kernel.";
        what << endl;
        what << "    Error: " << strerror(errno);
        what << " (" << errno << ")." << endl;
        throw runtime_error(what.str());
    }

    // Set id
    if (id < 0) {
        this->id = this->idseq;
    } else {
        this->id = id;
    }
    this->idseq++;
}

VIfaceImpl::~VIfaceImpl()
{
    if (close(this->queues.rx) ||
        close(this->queues.tx) ||
        close(this->kernel_socket) ||
        close(this->kernel_socket_ipv6)) {
        ostringstream what;
        what << "--- Unable to close file descriptors for interface ";
        what << this->name << "." << endl;
        what << "    Error: " << strerror(errno);
        what << " (" << errno << ")." << endl;
        throw runtime_error(what.str());
    }
}

void VIfaceImpl::setMAC(string mac)
{
    vector<uint8_t> mac_bin = utils::parse_mac(mac);
    this->mac = mac;
    return;
}

string VIfaceImpl::getMAC() const
{
    // Read interface flags
    struct ifreq ifr;
    read_flags(this->kernel_socket, this->name, ifr);

    if (ioctl(this->kernel_socket, SIOCGIFHWADDR, &ifr) != 0) {
        ostringstream what;
        what << "--- Unable to get MAC addr for " << this->name << "." << endl;
        what << "    Error: " << strerror(errno);
        what << " (" << errno << ")." << endl;
        throw runtime_error(what.str());
    }

    // Convert binary MAC address to string
    ostringstream addr;
    addr << hex << std::setfill('0');
    for (int i = 0; i < 6; i++) {
        addr << setw(2) << (unsigned int) (0xFF & ifr.ifr_hwaddr.sa_data[i]);
        if (i != 5) {
            addr << ":";
        }
    }
    return addr.str();
}

void VIfaceImpl::setIPv4(string ipv4)
{
    // Validate format
    struct in_addr addr;

    if (!inet_pton(AF_INET, ipv4.c_str(), &addr)) {
        ostringstream what;
        what << "--- Invalid IPv4 address (" << ipv4 << ") for ";
        what << this->name << "." << endl;
        throw invalid_argument(what.str());
    }

    this->ipv4 = ipv4;
    return;
}

string VIfaceImpl::ioctlGetIPv4(unsigned long request) const
{
    ostringstream what;

    // Read interface flags
    struct ifreq ifr;
    read_flags(this->kernel_socket, this->name, ifr);

    if (ioctl(this->kernel_socket, request, &ifr) != 0) {
        what << "--- Unable to get IPv4 for " << this->name << "." << endl;
        what << "    Error: " << strerror(errno);
        what << " (" << errno << ")." << endl;
        throw runtime_error(what.str());
    }

    // Convert binary IP address to string
    char addr[INET_ADDRSTRLEN];
    memset(&addr, 0, sizeof(addr));

    struct sockaddr_in* ipaddr = (struct sockaddr_in*) &ifr.ifr_addr;
    if (inet_ntop(AF_INET, &(ipaddr->sin_addr), addr, sizeof(addr)) == NULL) {
        what << "--- Unable to convert IPv4 for " << this->name << "." << endl;
        what << "    Error: " << strerror(errno);
        what << " (" << errno << ")." << endl;
        throw runtime_error(what.str());
    }

    return string(addr);
}

string VIfaceImpl::getIPv4() const
{
    return this->ioctlGetIPv4(SIOCGIFADDR);
}

void VIfaceImpl::setIPv4Netmask(string netmask)
{
    // Validate format
    struct in_addr addr;

    if (!inet_pton(AF_INET, netmask.c_str(), &addr)) {
        ostringstream what;
        what << "--- Invalid IPv4 netmask (" << netmask << ") for ";
        what << this->name << "." << endl;
        throw invalid_argument(what.str());
    }

    this->netmask = netmask;
    return;
}

string VIfaceImpl::getIPv4Netmask() const
{
    return this->ioctlGetIPv4(SIOCGIFNETMASK);
}

void VIfaceImpl::setIPv4Broadcast(string broadcast)
{
    // Validate format
    struct in_addr addr;

    if (!inet_pton(AF_INET, broadcast.c_str(), &addr)) {
        ostringstream what;
        what << "--- Invalid IPv4 address (" << broadcast << ") for ";
        what << this->name << "." << endl;
        throw invalid_argument(what.str());
    }

    this->broadcast = broadcast;
    return;
}

string VIfaceImpl::getIPv4Broadcast() const
{
    return this->ioctlGetIPv4(SIOCGIFBRDADDR);
}

void VIfaceImpl::setMTU(uint mtu)
{
    ostringstream what;

    if (mtu < ETH_HLEN) {
        what << "--- MTU " << mtu << " too small (< " << ETH_HLEN << ").";
        what << endl;
        throw invalid_argument(what.str());
    }

    // Are we sure about this upper validation?
    // lo interface reports this number for its MTU
    if (mtu > 65536) {
        what << "--- MTU " << mtu << " too large (> 65536)." << endl;
        throw invalid_argument(what.str());
    }

    this->mtu = mtu;

    return;
}

void VIfaceImpl::setIPv6(set<string> const& ipv6s)
{
    // Validate format
    struct in6_addr addr6;

    for (auto & ipv6 : ipv6s) {
        if (!inet_pton(AF_INET6, ipv6.c_str(), &addr6)) {
            ostringstream what;
            what << "--- Invalid IPv6 address (" << ipv6 << ") for ";
            what << this->name << "." << endl;
            throw invalid_argument(what.str());
        }
    }

    this->ipv6s = ipv6s;
    return;
}

set<string> VIfaceImpl::getIPv6() const
{
    // Return set
    set<string> result;

    // Buffer to store string representation of the address
    char buff[INET6_ADDRSTRLEN];
    memset(&buff, 0, sizeof(buff));

    // Cast pointer to ipv6 address
    struct sockaddr_in6* addr;

    // Pointers to list head and current node
    struct ifaddrs *head;
    struct ifaddrs *node;

    // Get list of interfaces
    if (getifaddrs(&head) == -1) {
        ostringstream what;
        what << "--- Failed to get list of interface addresses." << endl;
        what << "    Error: " << strerror(errno);
        what << " (" << errno << ")." << endl;
        throw runtime_error(what.str());
    }

    // Iterate list
    for (node = head; node != NULL; node = node->ifa_next) {
        if (node->ifa_addr == NULL) {
            continue;
        }
        if (node->ifa_addr->sa_family != AF_INET6) {
            continue;
        }

        if (string(node->ifa_name) != this->name) {
            continue;
        }

        // Convert IPv6 address to string representation
        addr = (struct sockaddr_in6*) node->ifa_addr;
        if (inet_ntop(AF_INET6, &(addr->sin6_addr), buff,
                      sizeof(buff)) == NULL) {
            ostringstream what;
            what << "--- Unable to convert IPv6 for " << this->name;
            what << "." << endl;
            what << "    Error: " << strerror(errno);
            what << " (" << errno << ")." << endl;
            throw runtime_error(what.str());
        }

        result.insert(string(buff));
    }
    freeifaddrs(head);

    return result;
}

uint VIfaceImpl::getMTU() const
{
    // Read interface flags
    struct ifreq ifr;
    read_flags(this->kernel_socket, this->name, ifr);

    if (ioctl(this->kernel_socket, SIOCGIFMTU, &ifr) != 0) {
        ostringstream what;
        what << "--- Unable to get MTU for " << this->name << "." << endl;
        what << "    Error: " << strerror(errno);
        what << " (" << errno << ")." << endl;
        throw runtime_error(what.str());
    }

    return ifr.ifr_mtu;
}

void VIfaceImpl::up()
{
    ostringstream what;

    if (this->isUp()) {
        what << "--- Virtual interface " << this->name;
        what << " is already up." << endl;
        what << "    up() Operation not permitted." << endl;
        throw runtime_error(what.str());
    }

    // Read interface flags
    struct ifreq ifr;
    read_flags(this->kernel_socket, this->name, ifr);

    // Set MAC address
    if (!this->mac.empty()) {
        vector<uint8_t> mac_bin = utils::parse_mac(this->mac);

        ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
        for (int i = 0; i < 6; i++) {
            ifr.ifr_hwaddr.sa_data[i] = mac_bin[i];
        }
        if (ioctl(this->kernel_socket, SIOCSIFHWADDR, &ifr) != 0) {
            what << "--- Unable to set MAC Address (" << this->mac;
            what << ") for " << this->name << "." << endl;
            what << "    Error: " << strerror(errno);
            what << " (" << errno << ")." << endl;
            throw runtime_error(what.str());
        }
    }

    // Set IPv4 related
    // FIXME: Refactor this, it's ugly :/
    struct sockaddr_in* addr = (struct sockaddr_in*) &ifr.ifr_addr;
    addr->sin_family = AF_INET;

    // Address
    if (!this->ipv4.empty()) {
        if (!inet_pton(AF_INET, this->ipv4.c_str(), &addr->sin_addr)) {
            what << "--- Invalid cached IPv4 address (" << this->ipv4;
            what << ") for " << this->name << "." << endl;
            what << "    Something really bad happened :/" << endl;
            throw runtime_error(what.str());
        }

        if (ioctl(this->kernel_socket, SIOCSIFADDR, &ifr) != 0) {
            what << "--- Unable to set IPv4 (" << this->ipv4;
            what << ") for " << this->name << "." << endl;
            what << "    Error: " << strerror(errno);
            what << " (" << errno << ")." << endl;
            throw runtime_error(what.str());
        }
    }

    // Netmask
    if (!this->netmask.empty()) {
        if (!inet_pton(AF_INET, this->netmask.c_str(), &addr->sin_addr)) {
            what << "--- Invalid cached IPv4 netmask (" << this->netmask;
            what << ") for " << this->name << "." << endl;
            what << "    Something really bad happened :/" << endl;
            throw runtime_error(what.str());
        }

        if (ioctl(this->kernel_socket, SIOCSIFNETMASK, &ifr) != 0) {
            what << "--- Unable to set IPv4 netmask (" << this->netmask;
            what << ") for " << this->name << "." << endl;
            what << "    Error: " << strerror(errno);
            what << " (" << errno << ")." << endl;
            throw runtime_error(what.str());
        }
    }

    // Broadcast
    if (!this->broadcast.empty()) {
        if (!inet_pton(AF_INET, this->broadcast.c_str(), &addr->sin_addr)) {
            what << "--- Invalid cached IPv4 broadcast (" << this->broadcast;
            what << ") for " << this->name << "." << endl;
            what << "    Something really bad happened :/" << endl;
            throw runtime_error(what.str());
        }

        if (ioctl(this->kernel_socket, SIOCSIFBRDADDR, &ifr) != 0) {
            what << "--- Unable to set IPv4 broadcast (" << this->broadcast;
            what << ") for " << this->name << "." << endl;
            what << "    Error: " << strerror(errno);
            what << " (" << errno << ")." << endl;
            throw runtime_error(what.str());
        }
    }

    // Set IPv6 related
    // FIXME: Refactor this, it's ugly :/
    if (!this->ipv6s.empty()) {
        struct in6_ifreq ifr6;
        memset(&ifr6, 0, sizeof(struct in6_ifreq));

        // Get interface index
        if (ioctl(this->kernel_socket, SIOGIFINDEX, &ifr) < 0) {
            what << "--- Unable to get interface index for (";
            what << this->name << "." << endl;
            what << "    Something really bad happened :/" << endl;
            throw runtime_error(what.str());
        }
        ifr6.ifr6_ifindex = ifr.ifr_ifindex;
        ifr6.ifr6_prefixlen = 64;

        for (auto & ipv6 : this->ipv6s) {
            // Parse IPv6 address into IPv6 address structure
            if (!inet_pton(AF_INET6, ipv6.c_str(), &ifr6.ifr6_addr)) {
                what << "--- Invalid cached IPv6 address (" << ipv6;
                what << ") for " << this->name << "." << endl;
                what << "    Something really bad happened :/" << endl;
                throw runtime_error(what.str());
            }

            // Set IPv6 address
            if (ioctl(this->kernel_socket_ipv6, SIOCSIFADDR, &ifr6) < 0) {
                what << "--- Unable to set IPv6 address (" << ipv6;
                what << ") for " << this->name << "." << endl;
                what << "    Error: " << strerror(errno);
                what << " (" << errno << ")." << endl;
                throw runtime_error(what.str());
            }
        }
    }

    // Set MTU
    ifr.ifr_mtu = this->mtu;
    if (ioctl(this->kernel_socket, SIOCSIFMTU, &ifr) != 0) {
        what << "--- Unable to set MTU (" << this->mtu << ") for ";
        what << this->name << "." << endl;
        what << "    Error: " << strerror(errno);
        what << " (" << errno << ")." << endl;
        throw runtime_error(what.str());
    }
    this->pktbuff.resize(this->mtu);

    // Bring-up interface
    ifr.ifr_flags |= IFF_UP;
    if (ioctl(this->kernel_socket, SIOCSIFFLAGS, &ifr) != 0) {
        what << "--- Unable to bring-up interface " << this->name;
        what << "." << endl;
        what << "    Error: " << strerror(errno);
        what << " (" << errno << ")." << endl;
        throw runtime_error(what.str());
    }

    return;
}

void VIfaceImpl::down() const
{
    // Read interface flags
    struct ifreq ifr;
    read_flags(this->kernel_socket, this->name, ifr);

    // Bring-down interface
    ifr.ifr_flags &= ~IFF_UP;
    if (ioctl(this->kernel_socket, SIOCSIFFLAGS, &ifr) != 0) {
        ostringstream what;
        what << "--- Unable to bring-down interface " << this->name;
        what << "." << endl;
        what << "    Error: " << strerror(errno);
        what << " (" << errno << ")." << endl;
        throw runtime_error(what.str());
    }

    return;
}

bool VIfaceImpl::isUp() const
{
    // Read interface flags
    struct ifreq ifr;
    read_flags(this->kernel_socket, this->name, ifr);

    return (ifr.ifr_flags & IFF_UP) != 0;
}

vector<uint8_t> VIfaceImpl::receive()
{
    // Read packet into our buffer
    int nread = read(this->queues.rx, &(this->pktbuff[0]), this->mtu);

    // Handle errors
    if (nread == -1) {
        // Nothing was read for this fd (non-blocking).
        // This could happen, as http://linux.die.net/man/2/select states:
        //
        //    Under Linux, select() may report a socket file descriptor as
        //    "ready for reading", while nevertheless a subsequent read
        //    blocks. This could for example happen when data has arrived
        //    but upon examination has wrong checksum and is discarded.
        //    There may be other circumstances in which a file descriptor
        //    is spuriously reported as ready. Thus it may be safer to
        //    use O_NONBLOCK on sockets that should not block.
        //
        // I know this is not a socket, but the "There may be other
        // circumstances in which a file descriptor is spuriously reported
        // as ready" warns it, and so, it better to do this that to have
        // an application that frozes for no apparent reason.
        //
        if (errno == EAGAIN) {
            return vector<uint8_t>(0);
        }

        // Something bad happened
        ostringstream what;
        what << "--- IO error while reading from " << this->name;
        what << "." << endl;
        what << "    Error: " << strerror(errno);
        what << " (" << errno << ")." << endl;
        throw runtime_error(what.str());
    }

    // Copy packet from buffer and return
    vector<uint8_t> packet(nread);
    packet.assign(&(this->pktbuff[0]), &(this->pktbuff[nread]));
    return packet;
}

void VIfaceImpl::send(vector<uint8_t>& packet) const
{
    ostringstream what;
    int size = packet.size();

    if (size < ETH_HLEN) {
        what << "--- Packet too small (" << size << ") ";
        what << "too small (< " << ETH_HLEN << ")." << endl;
        throw invalid_argument(what.str());
    }

    if (size > this->mtu) {
        what << "--- Packet too large (" << size << ") ";
        what << "for current MTU (> " << this->mtu << ")." << endl;
        throw invalid_argument(what.str());
    }

    // Write packet to TX queue
    int written = write(this->queues.tx, &packet[0], size);

    if (written != size) {
        what << "--- IO error while writting to " << this->name;
        what << "." << endl;
        what << "    Error: " << strerror(errno);
        what << " (" << errno << ")." << endl;
        throw runtime_error(what.str());
    }
    return;
}

std::set<std::string> VIfaceImpl::listStats()
{
    set<string> result;

    DIR* dir;
    struct dirent* ent;
    ostringstream what;
    string path = "/sys/class/net/" + this->name + "/statistics/";

    // Open directory
    if ((dir = opendir(path.c_str())) == NULL) {
        what << "--- Unable to open statistics folder for interface ";
        what << this->name << ":" << endl;
        what << "    " << path << endl;
        what << "    Error: " << strerror(errno);
        what << " (" << errno << ")." << endl;
        throw runtime_error(what.str());
    }

    // List files
    while ((ent = readdir(dir)) != NULL) {
        string entry(ent->d_name);

        // Ignore current, parent and hidden files
        if (entry[0] != '.') {
            result.insert(entry);
        }
    }

    // Close directory
    if (closedir(dir) != 0) {
        what << "--- Unable to close statistics folder for interface ";
        what << this->name << ":" << endl;
        what << "    " << path << endl;
        what << "    Error: " << strerror(errno);
        what << " (" << errno << ")." << endl;
        throw runtime_error(what.str());
    }

    // Update cache
    this->stats_keys_cache = result;

    return result;
}

uint64_t VIfaceImpl::readStatFile(string const& stat)
{
    ostringstream what;

    // If no cache exists, create it.
    if (this->stats_keys_cache.empty()) {
        this->listStats();
    }

    // Check if stat is valid
    if (stats_keys_cache.find(stat) == stats_keys_cache.end()) {
        what << "--- Unknown statistic " << stat;
        what << " for interface " << this->name << endl;
        throw runtime_error(what.str());
    }

    // Open file
    string path = "/sys/class/net/" + this->name + "/statistics/" + stat;
    ifstream statf(path);
    uint64_t value;

    // Check file open
    if (!statf.is_open()) {
        what << "--- Unable to open statistics file " << path;
        what << " for interface " << this->name << endl;
        throw runtime_error(what.str());
    }

    // Read content into value
    statf >> value;

    // close file
    statf.close();

    // Create entry if this stat wasn't cached
    if (this->stats_cache.find(stat) == this->stats_cache.end()) {
        this->stats_cache[stat] = 0;
    }

    return value;
}

uint64_t VIfaceImpl::readStat(std::string const& stat)
{
    uint64_t value = this->readStatFile(stat);

    // Return value minus the cached value
    return value - this->stats_cache[stat];
}

void VIfaceImpl::clearStat(std::string const& stat)
{
    uint64_t value = this->readStatFile(stat);

    // Set current value as cache
    this->stats_cache[stat] = value;
    return;
}


void dispatch(set<VIface*>& ifaces, dispatcher_cb callback, int millis)
{
    int fd = -1;
    int fdsread = -1;
    int nfds = -1;
    struct timeval tv;
    struct timeval* tvp = NULL;

    // Check non-empty set
    if (ifaces.empty()) {
        ostringstream what;
        what << "--- Empty virtual interfaces set" << endl;
        throw invalid_argument(what.str());
    }

    // Setup timeout
    if (millis >= 0) {
        tvp = &tv;
    }

    // Store mapping between file descriptors and interface it belongs
    map<int,VIface*> reverse_id;

    // Create and clear set of file descriptors
    fd_set rfds;

    // Create map of file descriptors and get maximum file descriptor for
    // select call
    for (auto iface : ifaces) {
        // Store identity
        fd = iface->pimpl->getRX();
        reverse_id[fd] = iface;

        // Get maximum file descriptor
        if (fd > nfds) {
            nfds = fd;
        }
    }
    nfds++;

    // Perform select system call
    while (true) {
        // Re-create set
        FD_ZERO(&rfds);
        for (auto iface : ifaces) {
            fd = iface->pimpl->getRX();
            FD_SET(fd, &rfds);
        }

        // Re-set timeout
        if (tvp != NULL) {
            tv.tv_sec = millis / 1000;
            tv.tv_usec = (millis % 1000) * 1000;
        }

        fdsread = select(nfds, &rfds, NULL, NULL, tvp);

        // Check if select error
        if (fdsread == -1) {
            // A signal was caught. Return.
            if (errno == EINTR) {
                return;
            }

            // Something bad happened
            ostringstream what;
            what << "--- Unknown error in select() system call: ";
            what << fdsread << "." << endl;
            what << "    Error: " << strerror(errno);
            what << " (" << errno << ")." << endl;
            throw runtime_error(what.str());
        }

        // Check if timeout
        if (tvp != NULL && fdsread == 0) {
            return;
        }

        // Iterate all active file descriptors for reading
        for (auto &pair : reverse_id) {
            // Check if fd wasn't marked in select as available
            if (!FD_ISSET(pair.first, &rfds)) {
                continue;
            }

            // File descriptor is ready, perform read and dispatch
            vector<uint8_t> packet = pair.second->receive();
            if (packet.size() == 0) {
                // Even if this is very unlikely, supposedly it can happen.
                // See receive() comments about this.
                continue;
            }

            // Dispatch packet
            if (!callback(
                    pair.second->getName(),
                    pair.second->getID(),
                    packet)) {
                return;
            }
        }
    }
}




/*============================================================================
   =   PIMPL IDIOM BUREAUCRACY
   =
   =   Starting this point there is not much relevant things...
   =   Stop scrolling...
 *============================================================================*/

VIface::VIface(string name, bool tap, int id) :
    pimpl(new VIfaceImpl(name, tap, id))
{}
VIface::~VIface() = default;

string VIface::getName() const {
    return this->pimpl->getName();
}

uint VIface::getID() const {
    return this->pimpl->getID();
}

void VIface::setMAC(string mac)
{
    return this->pimpl->setMAC(mac);
}

string VIface::getMAC() const
{
    return this->pimpl->getMAC();
}

void VIface::setIPv4(string ipv4)
{
    return this->pimpl->setIPv4(ipv4);
}

string VIface::getIPv4() const
{
    return this->pimpl->getIPv4();
}

void VIface::setIPv4Netmask(string netmask)
{
    return this->pimpl->setIPv4Netmask(netmask);
}

string VIface::getIPv4Netmask() const
{
    return this->pimpl->getIPv4Netmask();
}

void VIface::setIPv4Broadcast(string broadcast)
{
    return this->pimpl->setIPv4Broadcast(broadcast);
}

string VIface::getIPv4Broadcast() const
{
    return this->pimpl->getIPv4Broadcast();
}

void VIface::setIPv6(set<string> const& ipv6s)
{
    return this->pimpl->setIPv6(ipv6s);
}

set<string> VIface::getIPv6() const
{
    return this->pimpl->getIPv6();
}

void VIface::setMTU(uint mtu)
{
    return this->pimpl->setMTU(mtu);
}

uint VIface::getMTU() const
{
    return this->pimpl->getMTU();
}

void VIface::up()
{
    return this->pimpl->up();
}

void VIface::down() const
{
    return this->pimpl->down();
}

bool VIface::isUp() const
{
    return this->pimpl->isUp();
}

vector<uint8_t> VIface::receive()
{
    return this->pimpl->receive();
}

void VIface::send(vector<uint8_t>& packet) const
{
    return this->pimpl->send(packet);
}

std::set<std::string> VIface::listStats()
{
    return this->pimpl->listStats();
}

uint64_t VIface::readStat(std::string const& stat)
{
    return this->pimpl->readStat(stat);
}

void VIface::clearStat(std::string const& stat)
{
    return this->pimpl->clearStat(stat);
}
}
