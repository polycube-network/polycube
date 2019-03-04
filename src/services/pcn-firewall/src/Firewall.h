/*
 * Copyright 2017 The Polycube Authors
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

#include "../interface/FirewallInterface.h"

#include "polycube/services/cube.h"
#include "polycube/services/port.h"
#include "polycube/services/utils.h"

#include <arpa/inet.h>   //htonl() htons()
#include <netinet/in.h>  //IPPROTO_UDP IPPROTO_TCP
#include <spdlog/spdlog.h>
#include <bitset>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "Chain.h"
#include "Ports.h"
#include "SessionTable.h"
#include "defines.h"

using namespace io::swagger::server::model;

class Ports;
class Chain;

struct ct_k {
  uint32_t srcIp;
  uint32_t dstIp;
  uint8_t l4proto;
  uint16_t srcPort;
  uint16_t dstPort;
} __attribute__((packed));

struct ct_v {
  uint64_t ttl;
  uint8_t state;
  uint32_t sequence;
} __attribute__((packed));

class Firewall : public polycube::service::Cube<Ports>,
                 public FirewallInterface {
  friend class Ports;
  friend class Chain;
  friend class ChainRule;
  friend class ChainStats;
  friend class SessionTable;

 public:
  Firewall(const std::string name, const FirewallJsonObject &conf);
  virtual ~Firewall();
  std::string generate_code();
  std::vector<std::string> generate_code_vector();
  void packet_in(Ports &port, polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

  void update(const FirewallJsonObject &conf) override;
  FirewallJsonObject toJsonObject() override;

  /// <summary>
  ///
  /// </summary>
  std::shared_ptr<Chain> getChain(const ChainNameEnum &name) override;
  std::vector<std::shared_ptr<Chain>> getChainList() override;
  void addChain(const ChainNameEnum &name,
                const ChainJsonObject &conf) override;
  void addChainList(const std::vector<ChainJsonObject> &conf) override;
  void replaceChain(const ChainNameEnum &name,
                    const ChainJsonObject &conf) override;
  void delChain(const ChainNameEnum &name) override;
  void delChainList() override;

  /// <summary>
  /// Name for the ingress port, from which arrives traffic processed by INGRESS
  /// chain (by default it&#39;s the first port of the cube)
  /// </summary>
  std::string getIngressPort() override;
  void setIngressPort(const std::string &value) override;

  /// <summary>
  ///
  /// </summary>
  std::shared_ptr<SessionTable> getSessionTable(const std::string &src,
                                                const std::string &dst,
                                                const std::string &l4proto,
                                                const uint16_t &sport,
                                                const uint16_t &dport) override;
  std::vector<std::shared_ptr<SessionTable>> getSessionTableList() override;
  void addSessionTable(const std::string &src, const std::string &dst,
                       const std::string &l4proto, const uint16_t &sport,
                       const uint16_t &dport,
                       const SessionTableJsonObject &conf) override;
  void addSessionTableList(
      const std::vector<SessionTableJsonObject> &conf) override;
  void replaceSessionTable(const std::string &src, const std::string &dst,
                           const std::string &l4proto, const uint16_t &sport,
                           const uint16_t &dport,
                           const SessionTableJsonObject &conf) override;
  void delSessionTable(const std::string &src, const std::string &dst,
                       const std::string &l4proto, const uint16_t &sport,
                       const uint16_t &dport) override;
  void delSessionTableList() override;

  /// <summary>
  /// If Connection Tracking is enabled, all packets belonging to ESTABLISHED
  /// connections will be forwarded automatically. Default is ON.
  /// </summary>
  FirewallAcceptEstablishedEnum getAcceptEstablished() override;
  void setAcceptEstablished(
      const FirewallAcceptEstablishedEnum &value) override;

  /// <summary>
  /// Name for the egress port, from which arrives traffic processed by EGRESS
  /// chain (by default it&#39;s the second port of the cube)
  /// </summary>
  std::string getEgressPort() override;
  void setEgressPort(const std::string &value) override;

  /// <summary>
  /// Enables the Connection Tracking module. Mandatory if connection tracking
  /// rules are needed. Default is ON.
  /// </summary>
  FirewallConntrackEnum getConntrack() override;
  void setConntrack(const FirewallConntrackEnum &value) override;

  /// <summary>
  /// Entry of the ports table
  /// </summary>
  std::shared_ptr<Ports> getPorts(const std::string &name) override;
  std::vector<std::shared_ptr<Ports>> getPortsList() override;
  void addPorts(const std::string &name, const PortsJsonObject &conf) override;
  void addPortsList(const std::vector<PortsJsonObject> &conf) override;
  void replacePorts(const std::string &name,
                    const PortsJsonObject &conf) override;
  void delPorts(const std::string &name) override;
  void delPortsList() override;

  /// <summary>
  /// Interactive mode applies new rules immediately; if &#39;false&#39;, the
  /// command &#39;apply-rules&#39; has to be used to apply all the rules at
  /// once. Default is TRUE.
  /// </summary>
  bool getInteractive() override;
  void setInteractive(const bool &value) override;

  static const int maxRules = 8192;

 private:
  /*==========================
   *NESTED CLASSES DECLARATION
   *==========================*/
  class Program {
   protected:
    int index;
    std::string code;
    ChainNameEnum direction;
    std::map<std::string, Program *> hops;

    // The relationship between inner and outer must be explictly made in C++
    Firewall &firewall;

    std::string defaultActionString();

   public:
    virtual std::string getCode() = 0;

    void updateHop(int hopNumber, Program *hop,
                   ChainNameEnum hopDirection = ChainNameEnum::INVALID);

    Program *getHop(std::string hopName);

    int getIndex();

    bool reload();
    bool load();

    Program(const std::string &code, const int &index,
            const ChainNameEnum &direction, Firewall &outer);

    virtual ~Program();
  };

  class Parser : public Program {
   public:
    Parser(const int &index, Firewall &outer);
    ~Parser();
    std::string getCode();
  };

  class ChainForwarder : public Program {
   public:
    ChainForwarder(const int &index, Firewall &outer);
    ~ChainForwarder();
    std::string getCode();
  };

  class ConntrackLabel : public Program {
   public:
    ConntrackLabel(const int &index, Firewall &outer);
    ~ConntrackLabel();
    std::string getCode();
    std::vector<std::pair<ct_k, ct_v>> getMap();
  };

  class ConntrackMatch : public Program {
   public:
    ConntrackMatch(const int &index, const ChainNameEnum &direction,
                   Firewall &outer);
    ~ConntrackMatch();
    std::string getCode();
    bool updateTableValue(uint8_t status, const std::vector<uint64_t> &value);
    void updateMap(const std::map<uint8_t, std::vector<uint64_t>> &status);
  };

  class IpLookup : public Program {
   private:
    int type;  // SOURCE or DESTINATION

   public:
    IpLookup(const int &index, const ChainNameEnum &direction, const int &type,
             Firewall &outer);
    ~IpLookup() override;
    std::string getCode();
    void updateTableValue(uint8_t netmask, std::string ip,
                          const std::vector<uint64_t> &value);
    void updateTableValue(IpAddr ip, const std::vector<uint64_t> &value);
    void updateMap(const std::map<struct IpAddr, std::vector<uint64_t>> &ips);
  };

  class L4ProtocolLookup : public Program {
   public:
    L4ProtocolLookup(const int &index, const ChainNameEnum &direction,
                     Firewall &outer);
    ~L4ProtocolLookup();
    std::string getCode();
    bool updateTableValue(uint8_t proto, const std::vector<uint64_t> &value);
    void updateMap(std::map<int, std::vector<uint64_t>> &protocols);
  };

  class L4PortLookup : public Program {
   private:
    int type;  // SOURCE or DESTINATION

   public:
    L4PortLookup(const int &index, const ChainNameEnum &direction,
                 const int &type, Firewall &outer);
    ~L4PortLookup();
    std::string getCode();
    bool updateTableValue(uint16_t port, const std::vector<uint64_t> &value);
    void updateMap(const std::map<uint16_t, std::vector<uint64_t>> &ports);
  };

  class TcpFlagsLookup : public Program {
   public:
    static const uint8_t possibleFlagsCombinations[];
    TcpFlagsLookup(const int &index, const ChainNameEnum &direction,
                   Firewall &outer);
    ~TcpFlagsLookup();
    std::string getCode();
    bool updateTableValue(uint32_t flagMask,
                          const std::vector<uint64_t> &value);
    void updateMap(const std::vector<std::vector<uint64_t>> &flags);
  };

  class BitScan : public Program {
   public:
    BitScan(const int &index, const ChainNameEnum &direction, Firewall &outer);
    ~BitScan();
    void loadIndex64();
    std::string getCode();
  };

  class ActionLookup : public Program {
   public:
    ActionLookup(const int &index, const ChainNameEnum &direction,
                 Firewall &outer);
    ~ActionLookup();
    bool updateTableValue(int rule, int action);
    uint64_t getPktsCount(int ruleNumber);
    uint64_t getBytesCount(int ruleNumber);
    void flushCounters(int ruleNumber);
    std::string getCode();
  };

  class DefaultAction : public Program {
   public:
    DefaultAction(const int &index, Firewall &outer);
    ~DefaultAction();
    uint64_t getPktsCount(ChainNameEnum chain);
    uint64_t getBytesCount(ChainNameEnum chain);
    void flushCounters(ChainNameEnum chain);
    std::string getCode();
  };

  class ConntrackTableUpdate : public Program {
   public:
    ConntrackTableUpdate(const int &index, Firewall &outer);
    ~ConntrackTableUpdate();
    std::string getCode();
  };

  /*==========================
   *ATTRIBUTES DECLARATION
   *==========================*/

 public:
  bool interactive_ = true;
  bool transparent = true;

  uint8_t conntrackMode = ConntrackModes::MANUAL;

 private:
  // If false, disables all printk in the datapath
  bool trace = true;

  std::string ingressPort, egressPort;

  // Keeps the mapping between an index and the eBPF program, represented as a
  // child of the Program class. The index is <ModulesConstants, Direction>

  std::map<ChainNameEnum, Chain> chains_;
  std::map<std::pair<uint8_t, ChainNameEnum>, Firewall::Program *> programs;

  /*==========================
   *METHODS DECLARATION
   *==========================*/
  void reload_chain(ChainNameEnum chain);
  void reload_all();
  int getIngressPortIndex();
  int getEgressPortIndex();

  bool isContrackActive();

  /*==========================
   *UTILITY FUNCTIONS
   *==========================*/

  static int protocol_from_string_to_int(const std::string &proto);

  static void replaceAll(std::string &str, const std::string &from,
                         const std::string &to);

  template <typename Iterator>
  static std::string fromContainerToMapString(Iterator begin, Iterator end);
};

template <typename Iterator>
std::string Firewall::fromContainerToMapString(Iterator it, Iterator end) {
  std::string result = "{ [ ";
  for (; it != end; ++it) {
    result += /*"-" + */ std::to_string((*it));
    result += " ";
  }
  result += "] }";
  return result;
}
