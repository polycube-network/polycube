/**
* iptables API
* iptables API generated from iptables.yang
*
* OpenAPI spec version: 1.0.0
*
* NOTE: This class is auto generated by the swagger code generator program.
* https://github.com/polycube-network/swagger-codegen.git
* branch polycube
*/


// These methods have a default implementation. Your are free to keep it or add your own


#include "../Iptables.h"

std::shared_ptr<Ports> Iptables::getPorts(const std::string &name){
  return Ports::getEntry(*this, name);
}

std::vector<std::shared_ptr<Ports>> Iptables::getPortsList(){
  return Ports::get(*this);
}

void Iptables::addPorts(const std::string &name, const PortsJsonObject &conf){
  Ports::create(*this, name, conf);
}

void Iptables::addPortsList(const std::vector<PortsJsonObject> &conf){
  for(auto &i : conf){
    std::string name_ = i.getName();
    Ports::create(*this, name_,  i);
  }
}

void Iptables::replacePorts(const std::string &name, const PortsJsonObject &conf){
  Ports::removeEntry(*this, name);
  std::string name_ = conf.getName();
  Ports::create(*this, name_, conf);

}

void Iptables::delPorts(const std::string &name){
  Ports::removeEntry(*this, name);
}

void Iptables::delPortsList(){
  Ports::remove(*this);
}

std::shared_ptr<SessionTable> Iptables::getSessionTable(const std::string &src, const std::string &dst, const std::string &l4proto, const uint16_t &sport, const uint16_t &dport){
  return SessionTable::getEntry(*this, src, dst, l4proto, sport, dport);
}

std::vector<std::shared_ptr<SessionTable>> Iptables::getSessionTableList(){
  return SessionTable::get(*this);
}

void Iptables::addSessionTable(const std::string &src, const std::string &dst, const std::string &l4proto, const uint16_t &sport, const uint16_t &dport, const SessionTableJsonObject &conf){
  SessionTable::create(*this, src, dst, l4proto, sport, dport, conf);
}

void Iptables::addSessionTableList(const std::vector<SessionTableJsonObject> &conf){
  for(auto &i : conf){
    std::string src_ = i.getSrc();
    std::string dst_ = i.getDst();
    std::string l4proto_ = i.getL4proto();
    uint16_t sport_ = i.getSport();
    uint16_t dport_ = i.getDport();
    SessionTable::create(*this, src_, dst_, l4proto_, sport_, dport_,  i);
  }
}

void Iptables::replaceSessionTable(const std::string &src, const std::string &dst, const std::string &l4proto, const uint16_t &sport, const uint16_t &dport, const SessionTableJsonObject &conf){
  SessionTable::removeEntry(*this, src, dst, l4proto, sport, dport);
  std::string src_ = conf.getSrc();
  std::string dst_ = conf.getDst();
  std::string l4proto_ = conf.getL4proto();
  uint16_t sport_ = conf.getSport();
  uint16_t dport_ = conf.getDport();
  SessionTable::create(*this, src_, dst_, l4proto_, sport_, dport_, conf);

}

void Iptables::delSessionTable(const std::string &src, const std::string &dst, const std::string &l4proto, const uint16_t &sport, const uint16_t &dport){
  SessionTable::removeEntry(*this, src, dst, l4proto, sport, dport);
}

void Iptables::delSessionTableList(){
  SessionTable::remove(*this);
}

std::shared_ptr<Chain> Iptables::getChain(const ChainNameEnum &name){
  return Chain::getEntry(*this, name);
}

std::vector<std::shared_ptr<Chain>> Iptables::getChainList(){
  return Chain::get(*this);
}

void Iptables::addChain(const ChainNameEnum &name, const ChainJsonObject &conf){
  Chain::create(*this, name, conf);
}

void Iptables::addChainList(const std::vector<ChainJsonObject> &conf){
  for(auto &i : conf){
    ChainNameEnum name_ = i.getName();
    Chain::create(*this, name_,  i);
  }
}

void Iptables::replaceChain(const ChainNameEnum &name, const ChainJsonObject &conf){
  Chain::removeEntry(*this, name);
  ChainNameEnum name_ = conf.getName();
  Chain::create(*this, name_, conf);

}

void Iptables::delChain(const ChainNameEnum &name){
  Chain::removeEntry(*this, name);
}

void Iptables::delChainList(){
  Chain::remove(*this);
}



