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

#include "../Firewall.h"

Firewall::Program::Program(const std::string &code, const int &index,
                           const ChainNameEnum &direction, Firewall &outer)
    : firewall(outer) {
  this->code = code;
  this->index = index;
  this->direction = direction;
};

Firewall::Program::~Program() {
  if (index != 0) {
    // Deleting the program with index 0 causes an exception.
    firewall.del_program(index);
  }
}

void Firewall::Program::updateHop(int hopNumber, Program *hop,
                                  ChainNameEnum hopDirection) {
  std::string hopName = "_NEXT_HOP_";
  if (hopDirection == ChainNameEnum::INGRESS)
    hopName += "INGRESS_";
  else if (hopDirection == ChainNameEnum::EGRESS)
    hopName += "EGRESS_";
  hopName += std::to_string(hopNumber);
  hops[hopName] = hop;
}

int Firewall::Program::getIndex() {
  return index;
}

bool Firewall::Program::reload() {
  try {
    firewall.reload(getCode(), index);
  } catch (...) {
    return false;
  }
  return true;
}

bool Firewall::Program::load() {
  try {
    firewall.add_program(getCode(), index);
  } catch (std::runtime_error e) {
    std::cout << e.what() << std::endl;
    return false;
  }
  return true;
}

Firewall::Program *Firewall::Program::getHop(std::string hopName) {
  auto it = hops.find(hopName);
  if (it != hops.end()) {
    return it->second;
  } else {
    return nullptr;
  }
}

std::string Firewall::Program::defaultActionString() {
  /*The default action for every program is calling the default action module.*/
  return "call_ingress_program(ctx, " +
         std::to_string(3 + ModulesConstants::NR_MODULES * 4) +
         "); "
         "return RX_DROP;";
}
