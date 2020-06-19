/**
* policer API generated from policer.yang
*
* NOTE: This file is auto generated by polycube-codegen
* https://github.com/polycube-network/polycube-codegen
*/


/* Do not edit this file manually */

/*
* ContractBase.h
*
*
*/

#pragma once

#include "../serializer/ContractJsonObject.h"

#include "../serializer/ContractUpdateDataInputJsonObject.h"






#include <spdlog/spdlog.h>

using namespace polycube::service::model;

class Policer;

class ContractBase {
 public:
  
  ContractBase(Policer &parent);
  
  virtual ~ContractBase();
  virtual void update(const ContractJsonObject &conf);
  virtual ContractJsonObject toJsonObject();

  /// <summary>
  /// Identifier of the class of traffic, provided as packet metadata
  /// </summary>
  virtual uint32_t getTrafficClass() = 0;

  /// <summary>
  /// Action applied to traffic of the contract: PASS &#x3D; Let all the traffic pass without limitations; LIMIT &#x3D; Apply rate and burst limits to selected traffic; DROP &#x3D; Drop all the traffic
  /// </summary>
  virtual ActionTypeEnum getAction() = 0;

  /// <summary>
  /// Maximum average traffic rate (in kbit/s)
  /// </summary>
  virtual uint64_t getRateLimit() = 0;

  /// <summary>
  /// Maximum size of a burst of packets (in kbits)
  /// </summary>
  virtual uint64_t getBurstLimit() = 0;
  virtual void updateData(ContractUpdateDataInputJsonObject input) = 0;

  std::shared_ptr<spdlog::logger> logger();
 protected:
  Policer &parent_;
};