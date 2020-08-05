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

#include "base/FirewallBase.h"

#include "polycube/services/transparent_cube.h"
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
#include "SessionTable.h"
#include "defines.h"

using namespace polycube::service::model;

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
  uint8_t ipRev;
  uint8_t portRev;
  uint32_t sequence;
} __attribute__((packed));

class Firewall : public FirewallBase {
  friend class Chain;
  friend class ChainRule;
  friend class ChainStats;
  friend class SessionTable;

 public:
  Firewall(const std::string name, const FirewallJsonObject &conf);
  virtual ~Firewall();
  void packet_in(polycube::service::Direction direction,
                 polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

  /// <summary>
  ///
  /// </summary>
  std::shared_ptr<Chain> getChain(const ChainNameEnum &name) override;
  std::vector<std::shared_ptr<Chain>> getChainList() override;
  void addChain(const ChainNameEnum &name,
                const ChainJsonObject &conf) override;
  void delChain(const ChainNameEnum &name) override;
  void delChainList() override;

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
  /// Enables the Connection Tracking module. Mandatory if connection tracking
  /// rules are needed. Default is ON.
  /// </summary>
  FirewallConntrackEnum getConntrack() override;
  void setConntrack(const FirewallConntrackEnum &value) override;

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

    polycube::service::ProgramType getProgramType();

    std::mutex program_mutex_;

  public:
    Program(const std::string &code, const int &index,
            const ChainNameEnum &direction, Firewall &outer);
    virtual ~Program();

    virtual std::string getCode() = 0;

    void updateHop(int hopNumber, Program *hop,
                   ChainNameEnum hopDirection = ChainNameEnum::INVALID);

    Program *getHop(std::string hopName);

    int getIndex();

    bool reload();
    bool load();

   private:
    std::string getAllCode();
  };

  class Parser : public Program {
   public:
    Parser(const int &index, const ChainNameEnum &direction, Firewall &outer);
    ~Parser();
    std::string getCode();
  };

  class Horus : public Program {
  public:

      Horus(const int &index, Firewall &outer, const ChainNameEnum &direction,
              const std::map<struct HorusRule, struct HorusValue> &horus);
      ~Horus();

      std::string getCode();
      std::string defaultActionString();

      uint64_t getPktsCount(int rule_number);
      uint64_t getBytesCount(int rule_number);

      void flushCounters(int rule_number);
      void updateTableValue(struct HorusRule horus_key,
                            struct HorusValue horus_value);
      void updateMap(const std::map<struct HorusRule, struct HorusValue> &horus);

  private:
      std::map<struct HorusRule, struct HorusValue> horus_;
  };

    class ChainForwarder : public Program {
   public:
    ChainForwarder(const int &index, const ChainNameEnum &direction,
                   Firewall &outer);
    ~ChainForwarder();
    std::string getCode();
  };

  class ConntrackLabel : public Program {
   public:
    ConntrackLabel(const int &index, const ChainNameEnum &direction, Firewall &outer);
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
   public:
    L4PortLookup(const int &index, const ChainNameEnum &direction,
                 const int &type, Firewall &outer);
    L4PortLookup(const int &index, const ChainNameEnum &chain, const int &type,
                 Firewall &outer,
                 const std::map<uint16_t, std::vector<uint64_t>> &ports);
    ~L4PortLookup();
    std::string getCode();
    bool updateTableValue(uint16_t port, const std::vector<uint64_t> &value);
    void updateMap(const std::map<uint16_t, std::vector<uint64_t>> &ports);

  private:
      int type;  // SOURCE or DESTINATION
      bool wildcard_rule_;
      std::string wildcard_string_;
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
    DefaultAction(const int &index, const ChainNameEnum &direction,
                  Firewall &outer);
    ~DefaultAction();
    uint64_t getPktsCount();
    uint64_t getBytesCount();
    void flushCounters();
    std::string getCode();
  };

  class ConntrackTableUpdate : public Program {
   public:
    ConntrackTableUpdate(const int &index, const ChainNameEnum &direction,
                         Firewall &outer);
    ~ConntrackTableUpdate();
    std::string getCode();

    void updateTimestamp();
    void updateTimestampTimer();
    void quitAndJoin();

    std::thread timestamp_update_thread_;
    std::atomic<bool> quit_thread_;
  };

  /*==========================
   *ATTRIBUTES DECLARATION
   *==========================*/

 public:
  uint8_t conntrackMode = ConntrackModes::MANUAL;

 private:
  // Keeps the mapping between an index and the eBPF program, represented as a
  // child of the Program class. The index is <ModulesConstants, Direction>

  std::map<ChainNameEnum, Chain> chains_;
  std::vector<Firewall::Program *> ingress_programs;
  std::vector<Firewall::Program *> egress_programs;

  // HORUS optimization enabled by current rule set
  bool horus_runtime_enabled_ingress_ = false;
  bool horus_runtime_enabled_egress_ = false;

  bool horus_enabled = true;

  // are we on swap or regular horus program index
  bool horus_swap_ingress_ = false;
  bool horus_swap_egress_ = false;

  /*==========================
   *METHODS DECLARATION
   *==========================*/
  void reload_chain(ChainNameEnum chain);
  void reload_all();

  bool isContrackActive();

  /*==========================
   *UTILITY FUNCTIONS
   *==========================*/

  static int protocol_from_string_to_int(const std::string &proto);

  static void replaceAll(std::string &str, const std::string &from,
                         const std::string &to);

  template <typename Iterator>
  static std::string fromContainerToMapString(Iterator begin, Iterator end,
                                              const std::string open,
                                              const std::string close,
                                              const std::string separator);
};

template <typename Iterator>
std::string Firewall::fromContainerToMapString(Iterator it, Iterator end,
                                               const std::string open,
                                               const std::string close,
                                               const std::string separator) {
  std::string result = open;
  bool first = true;
  int cnt = 0;
  for (; it != end; ++it) {
    cnt++;
    if (!first)
      result += separator;
    first = false;
    result += /*"-" + */ std::to_string((*it));
  }
  result += close;
  return result;
}
