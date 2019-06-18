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
    : firewall(outer), code(code), index(index), direction(direction) {}

Firewall::Program::~Program() {
  firewall.del_program(index, getProgramType());
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
    firewall.reload(getAllCode(), index, getProgramType());
  } catch (...) {
    return false;
  }
  return true;
}

bool Firewall::Program::load() {
  try {
    firewall.add_program(getAllCode(), index, getProgramType());
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
  return "call_next_program(ctx, " +
         std::to_string(ModulesConstants::DEFAULTACTION) +
         "); " +
         "return RX_DROP;";
}

polycube::service::ProgramType Firewall::Program::getProgramType() {
  switch (direction) {
  case ChainNameEnum::INGRESS:
    return polycube::service::ProgramType::INGRESS;
  case ChainNameEnum::EGRESS:
    return polycube::service::ProgramType::EGRESS;
  default:
    throw std::runtime_error("getProgramType(): Invalid chain type");
  }
}

std::string Firewall::Program::getAllCode() {
  std::string base_code;

  if (direction == ChainNameEnum::INGRESS) {
    base_code += std::string("#define _INGRESS_LOGIC\n");
    base_code += "static void call_next_program(struct CTXTYPE *ctx, int next) { \
              call_ingress_program(ctx, next);  \
            }\n";
  } else {
    base_code += std::string("#define _EGRESS_LOGIC\n");
    base_code += "static void call_next_program(struct CTXTYPE *ctx, int next) { \
              call_egress_program(ctx, next);  \
            }\n";
  }

  std::string code = getCode();
  // TODO: remove direction and use _CHAIN_NAME
  std::string direction_to_replace =
      ChainJsonObject::ChainNameEnum_to_string(direction);
  replaceAll(code, "_DIRECTION", direction_to_replace);

  replaceAll(code, "_CHAIN_NAME", direction_to_replace);

  return base_code + code;
}
