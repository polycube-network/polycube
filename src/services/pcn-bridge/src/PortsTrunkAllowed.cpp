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

#include "PortsTrunkAllowed.h"
#include "Bridge.h"

PortsTrunkAllowed::PortsTrunkAllowed(PortsTrunk &parent,
                                     const PortsTrunkAllowedJsonObject &conf)
    : PortsTrunkAllowedBase(parent) {
  logger()->debug("Creating PortsTrunkAllowed instance");

  vlan_ = conf.getVlanid();
}

PortsTrunkAllowed::~PortsTrunkAllowed() {
  logger()->debug("Destroying PortsTrunkAllowed instance for vlan {0}", vlan_);
}

uint16_t PortsTrunkAllowed::getVlanid() {
  return vlan_;
}
