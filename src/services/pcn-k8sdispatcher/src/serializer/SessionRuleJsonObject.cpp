/**
* k8sdispatcher API generated from k8sdispatcher.yang
*
* NOTE: This file is auto generated by polycube-codegen
* https://github.com/polycube-network/polycube-codegen
*/


/* Do not edit this file manually */



#include "SessionRuleJsonObject.h"
#include <regex>

namespace polycube {
namespace service {
namespace model {

SessionRuleJsonObject::SessionRuleJsonObject() {
  m_directionIsSet = false;
  m_srcIpIsSet = false;
  m_dstIpIsSet = false;
  m_srcPortIsSet = false;
  m_dstPortIsSet = false;
  m_protoIsSet = false;
  m_newIpIsSet = false;
  m_newPortIsSet = false;
  m_operationIsSet = false;
  m_originatingRuleIsSet = false;
}

SessionRuleJsonObject::SessionRuleJsonObject(const nlohmann::json &val) :
  JsonObjectBase(val) {
  m_directionIsSet = false;
  m_srcIpIsSet = false;
  m_dstIpIsSet = false;
  m_srcPortIsSet = false;
  m_dstPortIsSet = false;
  m_protoIsSet = false;
  m_newIpIsSet = false;
  m_newPortIsSet = false;
  m_operationIsSet = false;
  m_originatingRuleIsSet = false;


  if (val.count("direction")) {
    setDirection(string_to_SessionRuleDirectionEnum(val.at("direction").get<std::string>()));
  }

  if (val.count("src-ip")) {
    setSrcIp(val.at("src-ip").get<std::string>());
  }

  if (val.count("dst-ip")) {
    setDstIp(val.at("dst-ip").get<std::string>());
  }

  if (val.count("src-port")) {
    setSrcPort(val.at("src-port").get<uint16_t>());
  }

  if (val.count("dst-port")) {
    setDstPort(val.at("dst-port").get<uint16_t>());
  }

  if (val.count("proto")) {
    setProto(string_to_L4ProtoEnum(val.at("proto").get<std::string>()));
  }

  if (val.count("new-ip")) {
    setNewIp(val.at("new-ip").get<std::string>());
  }

  if (val.count("new-port")) {
    setNewPort(val.at("new-port").get<uint16_t>());
  }

  if (val.count("operation")) {
    setOperation(string_to_SessionRuleOperationEnum(val.at("operation").get<std::string>()));
  }

  if (val.count("originating-rule")) {
    setOriginatingRule(string_to_SessionRuleOriginatingRuleEnum(val.at("originating-rule").get<std::string>()));
  }
}

nlohmann::json SessionRuleJsonObject::toJson() const {
  nlohmann::json val = nlohmann::json::object();
  if (!getBase().is_null()) {
    val.update(getBase());
  }

  if (m_directionIsSet) {
    val["direction"] = SessionRuleDirectionEnum_to_string(m_direction);
  }

  if (m_srcIpIsSet) {
    val["src-ip"] = m_srcIp;
  }

  if (m_dstIpIsSet) {
    val["dst-ip"] = m_dstIp;
  }

  if (m_srcPortIsSet) {
    val["src-port"] = m_srcPort;
  }

  if (m_dstPortIsSet) {
    val["dst-port"] = m_dstPort;
  }

  if (m_protoIsSet) {
    val["proto"] = L4ProtoEnum_to_string(m_proto);
  }

  if (m_newIpIsSet) {
    val["new-ip"] = m_newIp;
  }

  if (m_newPortIsSet) {
    val["new-port"] = m_newPort;
  }

  if (m_operationIsSet) {
    val["operation"] = SessionRuleOperationEnum_to_string(m_operation);
  }

  if (m_originatingRuleIsSet) {
    val["originating-rule"] = SessionRuleOriginatingRuleEnum_to_string(m_originatingRule);
  }

  return val;
}

SessionRuleDirectionEnum SessionRuleJsonObject::getDirection() const {
  return m_direction;
}

void SessionRuleJsonObject::setDirection(SessionRuleDirectionEnum value) {
  m_direction = value;
  m_directionIsSet = true;
}

bool SessionRuleJsonObject::directionIsSet() const {
  return m_directionIsSet;
}



std::string SessionRuleJsonObject::SessionRuleDirectionEnum_to_string(const SessionRuleDirectionEnum &value){
  switch(value) {
    case SessionRuleDirectionEnum::INGRESS:
      return std::string("ingress");
    case SessionRuleDirectionEnum::EGRESS:
      return std::string("egress");
    default:
      throw std::runtime_error("Bad SessionRule direction");
  }
}

SessionRuleDirectionEnum SessionRuleJsonObject::string_to_SessionRuleDirectionEnum(const std::string &str){
  if (JsonObjectBase::iequals("ingress", str))
    return SessionRuleDirectionEnum::INGRESS;
  if (JsonObjectBase::iequals("egress", str))
    return SessionRuleDirectionEnum::EGRESS;
  throw std::runtime_error("SessionRule direction is invalid");
}
std::string SessionRuleJsonObject::getSrcIp() const {
  return m_srcIp;
}

void SessionRuleJsonObject::setSrcIp(std::string value) {
  m_srcIp = value;
  m_srcIpIsSet = true;
}

bool SessionRuleJsonObject::srcIpIsSet() const {
  return m_srcIpIsSet;
}



std::string SessionRuleJsonObject::getDstIp() const {
  return m_dstIp;
}

void SessionRuleJsonObject::setDstIp(std::string value) {
  m_dstIp = value;
  m_dstIpIsSet = true;
}

bool SessionRuleJsonObject::dstIpIsSet() const {
  return m_dstIpIsSet;
}



uint16_t SessionRuleJsonObject::getSrcPort() const {
  return m_srcPort;
}

void SessionRuleJsonObject::setSrcPort(uint16_t value) {
  m_srcPort = value;
  m_srcPortIsSet = true;
}

bool SessionRuleJsonObject::srcPortIsSet() const {
  return m_srcPortIsSet;
}



uint16_t SessionRuleJsonObject::getDstPort() const {
  return m_dstPort;
}

void SessionRuleJsonObject::setDstPort(uint16_t value) {
  m_dstPort = value;
  m_dstPortIsSet = true;
}

bool SessionRuleJsonObject::dstPortIsSet() const {
  return m_dstPortIsSet;
}



L4ProtoEnum SessionRuleJsonObject::getProto() const {
  return m_proto;
}

void SessionRuleJsonObject::setProto(L4ProtoEnum value) {
  m_proto = value;
  m_protoIsSet = true;
}

bool SessionRuleJsonObject::protoIsSet() const {
  return m_protoIsSet;
}



std::string SessionRuleJsonObject::L4ProtoEnum_to_string(const L4ProtoEnum &value){
  switch(value) {
    case L4ProtoEnum::TCP:
      return std::string("tcp");
    case L4ProtoEnum::UDP:
      return std::string("udp");
    case L4ProtoEnum::ICMP:
      return std::string("icmp");
    default:
      throw std::runtime_error("Bad SessionRule proto");
  }
}

L4ProtoEnum SessionRuleJsonObject::string_to_L4ProtoEnum(const std::string &str){
  if (JsonObjectBase::iequals("tcp", str))
    return L4ProtoEnum::TCP;
  if (JsonObjectBase::iequals("udp", str))
    return L4ProtoEnum::UDP;
  if (JsonObjectBase::iequals("icmp", str))
    return L4ProtoEnum::ICMP;
  throw std::runtime_error("SessionRule proto is invalid");
}
std::string SessionRuleJsonObject::getNewIp() const {
  return m_newIp;
}

void SessionRuleJsonObject::setNewIp(std::string value) {
  m_newIp = value;
  m_newIpIsSet = true;
}

bool SessionRuleJsonObject::newIpIsSet() const {
  return m_newIpIsSet;
}

void SessionRuleJsonObject::unsetNewIp() {
  m_newIpIsSet = false;
}

uint16_t SessionRuleJsonObject::getNewPort() const {
  return m_newPort;
}

void SessionRuleJsonObject::setNewPort(uint16_t value) {
  m_newPort = value;
  m_newPortIsSet = true;
}

bool SessionRuleJsonObject::newPortIsSet() const {
  return m_newPortIsSet;
}

void SessionRuleJsonObject::unsetNewPort() {
  m_newPortIsSet = false;
}

SessionRuleOperationEnum SessionRuleJsonObject::getOperation() const {
  return m_operation;
}

void SessionRuleJsonObject::setOperation(SessionRuleOperationEnum value) {
  m_operation = value;
  m_operationIsSet = true;
}

bool SessionRuleJsonObject::operationIsSet() const {
  return m_operationIsSet;
}

void SessionRuleJsonObject::unsetOperation() {
  m_operationIsSet = false;
}

std::string SessionRuleJsonObject::SessionRuleOperationEnum_to_string(const SessionRuleOperationEnum &value){
  switch(value) {
    case SessionRuleOperationEnum::XLATE_SRC:
      return std::string("xlate_src");
    case SessionRuleOperationEnum::XLATE_DST:
      return std::string("xlate_dst");
    default:
      throw std::runtime_error("Bad SessionRule operation");
  }
}

SessionRuleOperationEnum SessionRuleJsonObject::string_to_SessionRuleOperationEnum(const std::string &str){
  if (JsonObjectBase::iequals("xlate_src", str))
    return SessionRuleOperationEnum::XLATE_SRC;
  if (JsonObjectBase::iequals("xlate_dst", str))
    return SessionRuleOperationEnum::XLATE_DST;
  throw std::runtime_error("SessionRule operation is invalid");
}
SessionRuleOriginatingRuleEnum SessionRuleJsonObject::getOriginatingRule() const {
  return m_originatingRule;
}

void SessionRuleJsonObject::setOriginatingRule(SessionRuleOriginatingRuleEnum value) {
  m_originatingRule = value;
  m_originatingRuleIsSet = true;
}

bool SessionRuleJsonObject::originatingRuleIsSet() const {
  return m_originatingRuleIsSet;
}

void SessionRuleJsonObject::unsetOriginatingRule() {
  m_originatingRuleIsSet = false;
}

std::string SessionRuleJsonObject::SessionRuleOriginatingRuleEnum_to_string(const SessionRuleOriginatingRuleEnum &value){
  switch(value) {
    case SessionRuleOriginatingRuleEnum::POD_TO_EXT:
      return std::string("pod_to_ext");
    case SessionRuleOriginatingRuleEnum::NODEPORT_CLUSTER:
      return std::string("nodeport_cluster");
    default:
      throw std::runtime_error("Bad SessionRule originatingRule");
  }
}

SessionRuleOriginatingRuleEnum SessionRuleJsonObject::string_to_SessionRuleOriginatingRuleEnum(const std::string &str){
  if (JsonObjectBase::iequals("pod_to_ext", str))
    return SessionRuleOriginatingRuleEnum::POD_TO_EXT;
  if (JsonObjectBase::iequals("nodeport_cluster", str))
    return SessionRuleOriginatingRuleEnum::NODEPORT_CLUSTER;
  throw std::runtime_error("SessionRule originatingRule is invalid");
}

}
}
}
