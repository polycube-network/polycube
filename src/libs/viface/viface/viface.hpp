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

/**
 * @file viface.hpp
 * Main libviface header file.
 * Define the public interface for libviface.
 */

#ifndef _VIFACE_HPP
#define _VIFACE_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <functional>

#include "viface/config.hpp"

namespace viface
{
/**
 * @defgroup libviface Public Interface
 * @{
 */

class VIfaceImpl;
class VIface;

/**
 * Dispatch callback type to handle packet reception.
 *
 * @param[in]  name Name of the virtual interface that received the packet.
 * @param[in]  id Numeric ID assigned to the virtual interface.
 * @param[in]  packet Packet (if tun) or frame (if tap) as a binary blob
 *             (array of bytes).
 *
 * @return true if the dispatcher should continue processing or false to stop.
 */
typedef std::function<bool (std::string const& name, uint id,
                            std::vector<uint8_t>& packet)> dispatcher_cb;

/**
 * Dispatch function to handle packet reception for a group of interfaces.
 *
 * This function is implemented using an efficient select() system call to
 * monitor many network interfaces.
 *
 * @param[in]  ifaces a std::set of virtual interfaces to monitor.
 * @param[in]  callback a dispatcher_cb callback to be called to handle packet
 *             reception.
 * @param[in]  millis optional timeout value in milliseconds. > 0 means wait
 *             forever.
 *
 * @return always void.
 *         This call blocks forever UNLESS one of three situations are in place:
 *         - A signal is received, in which case is user's responsibility to
 *           recall the dispatch function or to stop execution.
 *         - A millis timeout value >= 0 is given and the timeout is reached.
 *         - The callback request termination by returning false.
 */
void dispatch(std::set<VIface*>& ifaces, dispatcher_cb callback,
              int millis = -1);

/**
 * Virtual Interface object.
 *
 * Creates a virtual network interface.
 */
class VIface
{
    private:

        std::unique_ptr<VIfaceImpl> pimpl;
        VIface(const VIface& other) = delete;
        VIface& operator=(VIface rhs) = delete;
        friend void dispatch(std::set<VIface*>& ifaces, dispatcher_cb callback,
                             int millis);

    public:

        /**
         * Create a VIface object with given name.
         *
         * @param[in]  name Name of the virtual interface. The placeholder %d
         *             can be used and a number will be assigned to it.
         * @param[in]  tap Tap device (default, true) or Tun device (false).
         * @param[in]  id Optional numeric id. If given id < 0 a sequential
         *             number will be given.
         */
        explicit VIface(
            std::string name = "viface%d",
            bool tap = true,
            int id = -1
            );
        ~VIface();

        /**
         * Getter method for virtual interface associated name.
         *
         * @return the name of the virtual interface.
         */
        std::string getName() const;

        /**
         * Getter method for virtual interface associated numeric id.
         *
         * @return the numeric id of the virtual interface.
         */
        uint getID() const;

        /**
         * Set the MAC address of the virtual interface.
         *
         * The format of the MAC address is verified, but is just until up()
         * is called that the library will try to attempt to write it.
         * If you don't provide a MAC address (the default) one will be
         * automatically assigned when bringing up the interface.
         *
         * @param[in]  mac New MAC address for this virtual interface in the
         *             form "d8:9d:67:d3:65:1f".
         *
         * @return always void.
         *         An exception is thrown in case of malformed argument.
         */
        void setMAC(std::string mac);

        /**
         * Getter method for virtual interface associated MAC Address.
         *
         * @return the current MAC address of the virtual interface.
         *         An empty string means no associated MAC address.
         */
        std::string getMAC() const;

        /**
         * Set the IPv4 address of the virtual interface.
         *
         * The format of the IPv4 address is verified, but is just until up()
         * is called that the library will try to attempt to write it.
         *
         * @param[in]  ipv4 New IPv4 address for this virtual interface in the
         *             form "172.17.42.1".
         *
         * @return always void.
         *         An exception is thrown in case of malformed argument.
         */
        void setIPv4(std::string ipv4);

        /**
         * Getter method for virtual interface associated IPv4 Address.
         *
         * @return the current IPv4 address of the virtual interface.
         *         An empty string means no associated IPv4 address.
         */
        std::string getIPv4() const;

        /**
         * Set the IPv4 netmask of the virtual interface.
         *
         * The format of the IPv4 netmask is verified, but is just until up()
         * is called that the library will try to attempt to write it.
         *
         * @param[in]  netmask New IPv4 netmask for this virtual interface in
         *             the form "255.255.255.0".
         *
         * @return always void.
         *         An exception is thrown in case of malformed argument.
         */
        void setIPv4Netmask(std::string netmask);

        /**
         * Getter method for virtual interface associated IPv4 netmask.
         *
         * @return the current IPv4 netmask of the virtual interface.
         *         An empty string means no associated IPv4 netmask.
         */
        std::string getIPv4Netmask() const;

        /**
         * Set the IPv4 broadcast address of the virtual interface.
         *
         * The format of the IPv4 broadcast address is verified, but is just
         * until up() is called that the library will try to attempt to write
         * it.
         *
         * @param[in]  broadcast New IPv4 broadcast address for this virtual
         *             interface in the form "172.17.42.255".
         *
         * @return always void.
         *         An exception is thrown in case of malformed argument.
         */
        void setIPv4Broadcast(std::string broadcast);

        /**
         * Getter method for virtual interface associated IPv4 broadcast
         * address.
         *
         * @return the current IPv4 broadcast address of the virtual interface.
         *         An empty string means no associated IPv4 broadcast address.
         */
        std::string getIPv4Broadcast() const;

        /**
         * Set the IPv6 addresses of the virtual interface.
         *
         * The format of the IPv6 addresses are verified, but is just until
         * up() is called that the library will try to attempt to write them.
         *
         * @param[in]  ipv6s New IPv6 addresses for this virtual interface in
         *             the form "::FFFF:204.152.189.116".
         *
         * @return always void.
         *         An exception is thrown in case of malformed argument.
         */
        void setIPv6(std::set<std::string> const& ipv6s);

        /**
         * Getter method for virtual interface associated IPv6 Addresses
         * (note the plural).
         *
         * @return the current IPv6 addresses of the virtual interface.
         *         An empty set means no associated IPv6 addresses.
         */
        std::set<std::string> getIPv6() const;

        /**
         * Set the MTU of the virtual interface.
         *
         * The range of the MTU is verified, but is just until up() is called
         * that the library will try to attempt to write it.
         *
         * @param[in]  mtu New MTU for this virtual interface.
         *
         * @return always void.
         *         An exception is thrown in case of bad range MTU
         *         ([68, 65536]).
         */
        void setMTU(uint mtu);

        /**
         * Getter method for virtual interface associated maximum transmission
         * unit (MTU).
         *
         * MTU is the size of the largest packet or frame that can be sent
         * using this interface.
         *
         * @return the current MTU of the virtual interface.
         */
        uint getMTU() const;

        /**
         * Bring up the virtual interface.
         *
         * This call will configure and bring up the interface.
         *
         * @return always void.
         *         Exceptions are thrown in case of configuration or bring-up
         *         failures.
         */
        void up();

        /**
         * Bring down the virtual interface.
         *
         * @return always void.
         *         Exceptions are thrown in case of misbehaviours.
         */
        void down() const;

        /**
         * Getter method for virtual interface associated maximum transmission
         * unit (MTU).
         *
         * MTU is the size of the largest packet or frame that can be sent
         * using this interface.
         *
         * @return the current MTU of the virtual interface.
         */
        bool isUp() const;

        /**
         * Receive a packet from the virtual interface.
         *
         * Note: Receive a packet from a virtual interface means that another
         * userspace program (or the kernel) sent a packet to the network
         * interface with the name of the instance of this class. If not packet
         * was available, and empty vector is returned.
         *
         * @return the packet (if tun) or frame (if tap) as a binary blob
         *         (array of bytes).
         */
        std::vector<uint8_t> receive();

        /**
         * Send a packet to this virtual interface.
         *
         * Note: Sending a packet to this virtual interface means that it
         * will reach any userspace program (or the kernel) listening for
         * packets in the interface with the name of the instance of this
         * class.
         *
         * @param[in]  packet Packet (if tun) or frame (if tap) to send as a
         *             binary blob (array of bytes).
         *
         * @return always void.
         *         Exceptions are thrown in case of misbehaviours. For example,
         *         size of the packet will be checked against MTU and minimal
         *         Ethernet 2 size. Another case can be write errors.
         */
        void send(std::vector<uint8_t>& packet) const;

        /**
         * List available statistics for this interface.
         *
         * @return set of statistics names.
         *         For example, {"rx_packets", "tx_bytes", ...}
         *         Exceptions can be thrown in case of generic IO errors.
         */
        std::set<std::string> listStats();

        /**
         * Read given statistic for this interface.
         *
         * @param[in]  stat statistic name. See listStats().
         *
         * @return current value of the given statistic.
         *         An exception is thrown if stat is unknown or in case of IO
         *         errors.
         */
        uint64_t readStat(std::string const& stat);

        /**
         * Clear given statistic for this interface.
         *
         * Please note that this feature is implemented in library as there is
         * currently no way to clear them (except by destroying the interface).
         * If you clear the statistics using this method, subsequent calls to
         * readStat() results will differ from, for example, those reported
         * by tools like ifconfig.
         *
         * @param[in]  stat statistic name. See listStats().
         *
         * @return always void.
         *         An exception is thrown if stat is unknown or in case of IO
         *         errors.
         */
        void clearStat(std::string const& stat);
};

/** @} */ // End of libviface
};
#endif // _VIFACE_HPP
