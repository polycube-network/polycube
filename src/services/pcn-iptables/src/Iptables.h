/*
 * Copyright 2018 The Polycube Authors
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

#include "../interface/IptablesInterface.h"

#include "polycube/services/cube.h"
#include "polycube/services/port.h"
#include "polycube/services/utils.h"

#include <arpa/inet.h>   //htonl() htons()
#include <netinet/in.h>  //IPPROTO_UDP IPPROTO_TCP
#include <bitset>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include <spdlog/spdlog.h>

#include "Chain.h"
#include "Ports.h"
#include "SessionTable.h"
#include "defines.h"

#include "../../../polycubed/src/utils/netlink.h"

#define REQUIRED_FIB_LOOKUP_KERNEL ("4.19.0")

using namespace io::swagger::server::model;

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

class Ports;
class Chain;

class Iptables : public polycube::service::Cube<Ports>,
                 public IptablesInterface {
  friend class Chain;
  friend class ChainRule;
  friend class ChainStats;
  friend class SessionTable;

 public:
  Iptables(const std::string name, const IptablesJsonObject &conf);
  virtual ~Iptables();

  void packet_in(Ports &port, polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

  /*APIs*/
  /// <summary>
  /// Enables the Connection Tracking module. Mandatory if connection tracking
  /// rules are needed. Default is ON.
  /// </summary>
  IptablesConntrackEnum getConntrack() override;
  void setConntrack(const IptablesConntrackEnum &value) override;

  /// <summary>
  /// Enables the HORUS optimization. Default is OFF.
  /// </summary>
  IptablesHorusEnum getHorus() override;
  void setHorus(const IptablesHorusEnum &value) override;

  /// <summary>
  /// Interactive mode applies new rules immediately; if &#39;false&#39;, the
  /// command &#39;apply-rules&#39; has to be used to apply all the rules at
  /// once. Default is TRUE.
  /// </summary>
  bool getInteractive() override;
  void setInteractive(const bool &value) override;

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

  std::shared_ptr<Chain> getChain(const ChainNameEnum &name) override;
  std::vector<std::shared_ptr<Chain>> getChainList() override;
  void addChain(const ChainNameEnum &name,
                const ChainJsonObject &conf) override;
  void addChainList(const std::vector<ChainJsonObject> &conf) override;
  void replaceChain(const ChainNameEnum &name,
                    const ChainJsonObject &conf) override;
  void delChain(const ChainNameEnum &name) override;
  void delChainList() override;

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

  void update(const IptablesJsonObject &conf) override;
  IptablesJsonObject toJsonObject() override;

  /*Utils*/
  void enableAcceptEstablished(Chain &chain);
  void disableAcceptEstablished(Chain &chain);

  void netlinkNotificationCallbackIptables();

  // Loop through interfaces visible in root ns, and connect them to
  // pcn-iptables
  void attachInterfaces();

  // Connect and create a port with same name of the interface
  bool connectPort(const std::string &name);

  // Disconnect a port with same name of the interface
  bool disconnectPort(const std::string &name);

  bool fibLookupEnabled();

  uint16_t interfaceNameToIndex(const std::string &interface_string);

  /*==========================
   *ATTRIBUTES DECLARATION
   *==========================*/

  // max rules number, due to algorithm and bpf programs size limit.
  // in order to avoid verifier to complain, we set this limit.
  static const int max_rules_ = 8192;

  std::mutex mutex_iptables_;

  int netlink_notification_index_;
  polycube::polycubed::Netlink &netlink_instance_iptables_;

  // interactive mode
  bool interactive_ = true;

  // HORUS optimization enabled by current rule set
  bool horus_runtime_enabled_ = false;
  bool horus_enabled = false;

  // are we on swap or regular horus program index
  bool horus_swap_ = false;

  bool fib_lookup_enabled_;
  bool fib_lookup_set_ = false;

 private:
  /*==========================
   *NESTED CLASSES DECLARATION
   *==========================*/
  class Program {
   protected:
    // returns "return RX_DROP" or "reutnr pcn_pkt_fwd(...)"
    std::string defaultActionString();

    // index in array of programs provided by polycube
    int index_;
    // Datapath
    std::string code_;
    // Input, Output, Forward
    ChainNameEnum chain_;
    // Program type (INGRESS/EGRESS Hook)
    ProgramType program_type_;
    // only used in chainforwarder
    std::map<std::string, std::shared_ptr<Program>> hops_;

    // The relationship between inner and outer must be explictly made in C++
    // in the costructor
    Iptables &iptables_;

    // program mutex is used to
    // 1) prevent multiple netlink notifications are processed at the same time
    // 2) prevent multiple load, reload at the same time of netlink notification
    // 3) mutex maps access/update
    std::mutex program_mutex_;

   public:
    // code = datapath (represented as string)
    // index = index the datapath is injected to
    // chain = (CHAIN NAME) INPUT/OUTPUT/FORWARD
    // outer = ref to iptables parent object

    // by default program loaded @ Object creation
    Program(const std::string &code, const int &index,
            const ChainNameEnum &chain, Iptables &outer,
            ProgramType type = ProgramType::INGRESS);

    // automatically unload program
    virtual ~Program();

    // virtual because each class has to define it
    // take datapath string, and replace string macro into the datapath
    virtual std::string getCode() = 0;

    // get index in prog array
    int getIndex();

    // encapsulate reload of polycube, without specitying index
    bool reload();

    // same as reload.
    bool load();

    // For a given Program, it generated a list of next hops_ with following
    // syntax
    // <_NEXT_HOP_<INPUT/FORWARD/OUTPUT>_<hop_number>
    // E.g. _NEXT_HOP_INPUT_1
    // used in chainforwarder
    void updateHop(int hop_number, std::shared_ptr<Program> hop,
                   ChainNameEnum hop_chain = ChainNameEnum::INVALID_INGRESS);

    Program *getHop(std::string hop_name);
  };

  class Parser : public Program {
   public:
    Parser(const int &index, Iptables &outer,
           const ProgramType type = ProgramType::INGRESS);
    ~Parser();

    std::string getCode();
    std::string defaultActionString(ChainNameEnum chain);  // Overrides

    void updateLocalIps();
  };

  class Horus : public Program {
   public:
    enum HorusType { HORUS_INGRESS, HORUS_EGRESS };

    Horus(const int &index, Iptables &outer,
          const std::map<struct HorusRule, struct HorusValue> &horus,
          HorusType t = HORUS_INGRESS);
    ~Horus();

    std::string getCode();
    std::string defaultActionString(ChainNameEnum chain);  // Overrides

    uint64_t getPktsCount(int rule_number);
    uint64_t getBytesCount(int rule_number);

    void flushCounters(int rule_number);
    void updateTableValue(struct HorusRule horus_key,
                          struct HorusValue horus_value);
    void updateMap(const std::map<struct HorusRule, struct HorusValue> &horus);

   private:
    HorusType type_;
    std::map<struct HorusRule, struct HorusValue> horus_;
  };

  class ChainSelector : public Program {
   public:
    ChainSelector(const int &index, Iptables &outer,
                  const ProgramType type = ProgramType::INGRESS);
    ~ChainSelector();

    std::string getCode();
    std::string defaultActionString(ChainNameEnum direction);  // Overrides

    uint64_t getDefaultPktsCount(ChainNameEnum chain);
    uint64_t getDefaultBytesCount(ChainNameEnum chain);

    void updateLocalIps();

    // chainselector is notified by netlink, when link status changes
    void netlinkNotificationCallbackChainSelector();

   private:
    static std::string removeNetFromIp(std::string ip);

    std::unordered_map<std::string, std::string> local_ips_;

    // use a new instance of netlink for each chainselector
    // prevent deadlocks with main instance present in polycubed
    polycube::polycubed::Netlink &netlink_instance_chainselector_;
    int netlink_notification_index_chainselector_;
  };

  class ChainForwarder : public Program {
   public:
    ChainForwarder(const int &index, Iptables &outer,
                   const ProgramType type = ProgramType::INGRESS);
    ~ChainForwarder();

    std::string getCode();
    std::string defaultActionString(ChainNameEnum chain);  // Overrides
  };

  class ConntrackLabel : public Program {
   public:
    ConntrackLabel(const int &index, Iptables &outer,
                   const ProgramType type = ProgramType::INGRESS);
    ~ConntrackLabel();

    std::string getCode();

    uint64_t getAcceptEstablishedPktsCount(ChainNameEnum chain);
    uint64_t getAcceptEstablishedBytesCount(ChainNameEnum chain);

    void flushCounters(ChainNameEnum chain, int rule_number);
    std::vector<std::pair<ct_k, ct_v>> getMap();
  };

  class ConntrackMatch : public Program {
   public:
    ConntrackMatch(const int &index, const ChainNameEnum &chain,
                   Iptables &outer);
    ~ConntrackMatch();

    std::string getCode();

    bool updateTableValue(uint8_t status, const std::vector<uint64_t> &value);
    void updateMap(const std::map<uint8_t, std::vector<uint64_t>> &status);
  };

  class IpLookup : public Program {
   public:
    IpLookup(const int &index, const ChainNameEnum &chain, const int &type,
             Iptables &outer);
    ~IpLookup() override;

    std::string getCode();

    void updateTableValue(uint8_t netmask, std::string ip,
                          const std::vector<uint64_t> &value);
    void updateTableValue(IpAddr ip, const std::vector<uint64_t> &value);
    void updateMap(const std::map<struct IpAddr, std::vector<uint64_t>> &ips);

   private:
    int type_;
  };

  class L4ProtocolLookup : public Program {
   public:
    L4ProtocolLookup(const int &index, const ChainNameEnum &chain,
                     Iptables &outer);
    ~L4ProtocolLookup();

    std::string getCode();

    bool updateTableValue(uint8_t proto, const std::vector<uint64_t> &value);
    void updateMap(std::map<int, std::vector<uint64_t>> &protocols);
  };

  class L4PortLookup : public Program {
   public:
    L4PortLookup(const int &index, const ChainNameEnum &chain, const int &type,
                 Iptables &outer);
    L4PortLookup(const int &index, const ChainNameEnum &chain, const int &type,
                 Iptables &outer,
                 const std::map<uint16_t, std::vector<uint64_t>> &ports);

    ~L4PortLookup();
    std::string getCode();

    bool updateTableValue(uint16_t port, const std::vector<uint64_t> &value);
    void updateMap(const std::map<uint16_t, std::vector<uint64_t>> &ports);

   private:
    int type_;  // SOURCE or DESTINATION
    bool wildcard_rule_;
    std::string wildcard_string_;
  };

  class InterfaceLookup : public Program {
   public:
    InterfaceLookup(const int &index, const ChainNameEnum &chain,
                    const int &type, Iptables &outer);
    InterfaceLookup(
        const int &index, const ChainNameEnum &chain, const int &type,
        Iptables &outer,
        const std::map<uint16_t, std::vector<uint64_t>> &interfaces);

    ~InterfaceLookup();
    std::string getCode();

    bool updateTableValue(uint16_t port, const std::vector<uint64_t> &value);
    void updateMap(const std::map<uint16_t, std::vector<uint64_t>> &interfaces);

   private:
    int type_;  // IN or OUT
    bool wildcard_rule_;
    std::string wildcard_string_;
  };

  class TcpFlagsLookup : public Program {
   public:
    static const uint8_t possible_flags_combinations_[];
    TcpFlagsLookup(const int &index, const ChainNameEnum &chain,
                   Iptables &outer);
    ~TcpFlagsLookup();

    std::string getCode();

    bool updateTableValue(uint32_t flag_mask,
                          const std::vector<uint64_t> &value);
    void updateMap(const std::vector<std::vector<uint64_t>> &flags);
  };

  class BitScan : public Program {
   public:
    BitScan(const int &index, const ChainNameEnum &chain, Iptables &outer);
    ~BitScan();

    std::string getCode();

    void loadIndex64();
  };

  class ActionLookup : public Program {
   public:
    ActionLookup(const int &index, const ChainNameEnum &chain, Iptables &outer);
    ~ActionLookup();

    std::string getCode();

    bool updateTableValue(int rule_number, int action);
    uint64_t getPktsCount(int rule_number);
    uint64_t getBytesCount(int ruleNumber);
    void flushCounters(int rule_number);
  };

  class ConntrackTableUpdate : public Program {
   public:
    ConntrackTableUpdate(const int &index, Iptables &outer,
                         const ProgramType program_type = ProgramType::INGRESS);
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

  uint8_t conntrack_mode_ = ConntrackModes::OFF;  // No Optimizations yet

  uint8_t conntrack_mode_input_ = ConntrackModes::OFF;  // No Optimizations yet
  uint8_t conntrack_mode_forward_ =
      ConntrackModes::OFF;                               // No Optimizations yet
  uint8_t conntrack_mode_output_ = ConntrackModes::OFF;  // No Optimizations yet

  bool accept_established_enabled_input_ = false;
  bool accept_established_enabled_forward_ = false;
  bool accept_established_enabled_output_ = false;

  std::map<ChainNameEnum, Chain> chains_;
  std::unordered_map<std::string, std::string> connected_ports_;

  // Keeps the mapping between an index and the eBPF program, represented as a
  // child of the Program class. The index is <ModulesConstants, Chain>
  // maintains table of programs
  std::map<std::pair<uint8_t, ChainNameEnum>,
           std::shared_ptr<Iptables::Program>>
      programs_;

  /*==========================
   *METHODS DECLARATION
   *==========================*/
  void reloadChain(ChainNameEnum chain);
  void reloadAll();

  bool isContrackActive();

  /*==========================
   *UTILITY FUNCTIONS
   *==========================*/
  static int protocolFromStringToInt(const std::string &proto);
  static void replaceAll(std::string &str, const std::string &from,
                         const std::string &to);
  template <typename Iterator>
  static std::string fromContainerToMapString(Iterator begin, Iterator end);
  template <typename Iterator>
  static std::string fromContainerToMapString(Iterator begin, Iterator end,
                                              const std::string open,
                                              const std::string close,
                                              const std::string separator);
};

template <typename Iterator>
std::string Iptables::fromContainerToMapString(Iterator it, Iterator end) {
  std::string result = "{ [ ";
  for (; it != end; ++it) {
    result += /*"-" + */ std::to_string((*it));
    result += " ";
  }
  result += "] }";
  return result;
}

template <typename Iterator>
std::string Iptables::fromContainerToMapString(Iterator it, Iterator end,
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
