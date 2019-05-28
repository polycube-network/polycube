/*
 * Copyright 2019 The Polycube Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <string>
#include "netlink.h"
#include "ns.h"

namespace polycube {
namespace polycubed {

class VethPeer {
  public:
    VethPeer();
    ~VethPeer();

    void set_name(const std::string &name);
    std::string get_name() const;

    void set_namespace(const std::string &ns);
    std::string get_namespace() const;

    void set_status(IFACE_STATUS status);
    void set_mac(const std::string &mac);
    void set_ip(const std::string &ip, const int prefix);

  private:
    std::string name_;
    std::string ns_;
};

class Veth {
  public:
    static Veth create(const std::string &peerA, const std::string &peerB);
    ~Veth();

    void remove();

    VethPeer& get_peerA();
    VethPeer& get_peerB();

  private:
    Veth(const std::string &peerA, const std::string &peerB);
    VethPeer peerA_;
    VethPeer peerB_;
};

}  // namespace polycubed
}  // namespace polycube
