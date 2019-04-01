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

// Program.ccp is the class others programs (Parser, Lookup, etc)..
// Inherits from.

// Definitions here
#include "../Iptables.h"

Iptables::Program::Program(const std::string &code, const int &index,
                           const ChainNameEnum &chain, Iptables &outer,
                           const ProgramType t)
    : iptables_(outer) {
  this->code_ = code;
  this->index_ = index;
  this->chain_ = chain;
  this->program_type_ = t;
};

Iptables::Program::~Program() {
  if (index_ != 0) {
    // Deleting the program with index 0 causes an exception.
    iptables_.del_program(index_, program_type_);
  }
}

void Iptables::Program::updateHop(int hop_number, std::shared_ptr<Program> hop,
                                  ChainNameEnum hop_chain) {
  std::string hop_name = "_NEXT_HOP_";
  if (hop_chain == ChainNameEnum::INPUT)
    hop_name += "INPUT_";
  if (hop_chain == ChainNameEnum::FORWARD)
    hop_name += "FORWARD_";
  if (hop_chain == ChainNameEnum::OUTPUT)
    hop_name += "OUTPUT_";
  hop_name += std::to_string(hop_number);
  hops_[hop_name] = hop;
}

int Iptables::Program::getIndex() {
  return index_;
}

bool Iptables::Program::reload() {
  std::lock_guard<std::mutex> guard(program_mutex_);
  try {
    iptables_.reload(getCode(), index_, program_type_);
  } catch (std::runtime_error e) {
    iptables_.logger()->error("reload err: {0}", e.what());
    return false;
  }
  return true;
}

bool Iptables::Program::load() {
  std::lock_guard<std::mutex> guard(program_mutex_);
  try {
    iptables_.add_program(getCode(), index_, program_type_);
  } catch (std::runtime_error e) {
    iptables_.logger()->error("load err: {0}", e.what());
    return false;
  }
  return true;
}

// Iptables::Program *Iptables::Program::getHop(std::string hopName) {
//  auto it = hops_.find(hopName);
//  if (it != hops_.end()) {
//    return it->second;
//  } else {
//    return nullptr;
//  }
//}

std::string Iptables::Program::defaultActionString() {
  if (iptables_.getChain(chain_)->getDefault() == ActionEnum::DROP) {
    return "return RX_DROP;";
  } else {
    std::string ret =
        "pcn_log(ctx, LOG_TRACE, \"Program Calling Default action "
        "ConntrackTableSave\"); \ncall_bpf_program(ctx, "
        "_CONNTRACKTABLEUPDATE);\n return RX_DROP;";
    if ((chain_ == ChainNameEnum::INPUT) ||
        (chain_ == ChainNameEnum::FORWARD)) {
      replaceAll(
          ret, "_CONNTRACKTABLEUPDATE",
          std::to_string(ModulesConstants::CONNTRACKTABLEUPDATE_INGRESS));
    } else {
      replaceAll(ret, "_CONNTRACKTABLEUPDATE",
                 std::to_string(ModulesConstants::CONNTRACKTABLEUPDATE_EGRESS));
    }
    return ret;
  }
}

// TODO
// Use a getCode common, that replaces following string by default. so it will
// be transparent for the inner class

// if (type == ProgramType::INGRESS){
// replaceAll(noMacroCode, "call_bpf_program", "call_ingress_program");
//} else if (type == ProgramType::EGRESS){
// replaceAll(noMacroCode, "call_bpf_program", "call_egress_program");
//}
