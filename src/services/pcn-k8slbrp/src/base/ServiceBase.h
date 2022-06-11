/**
* k8slbrp API generated from k8slbrp.yang
*
* NOTE: This file is auto generated by polycube-codegen
* https://github.com/polycube-network/polycube-codegen
*/


/* Do not edit this file manually */

/*
* ServiceBase.h
*
*
*/

#pragma once

#include "../serializer/ServiceJsonObject.h"

#include "../ServiceBackend.h"





#include <spdlog/spdlog.h>

using namespace polycube::service::model;

class K8slbrp;

class ServiceBase {
 public:
  
  ServiceBase(K8slbrp &parent);
  
  virtual ~ServiceBase();
  virtual void update(const ServiceJsonObject &conf);
  virtual ServiceJsonObject toJsonObject();

  /// <summary>
  /// Service name related to the backend server of the pool is connected to
  /// </summary>
  virtual std::string getName() = 0;
  virtual void setName(const std::string &value) = 0;

  /// <summary>
  /// Virtual IP (vip) of the service where clients connect to
  /// </summary>
  virtual std::string getVip() = 0;

  /// <summary>
  /// Port of the virtual server where clients connect to (this value is ignored in case of ICMP)
  /// </summary>
  virtual uint16_t getVport() = 0;

  /// <summary>
  /// Upper-layer protocol associated with a loadbalancing service instance. &#39;ALL&#39; creates an entry for all the supported protocols
  /// </summary>
  virtual ServiceProtoEnum getProto() = 0;

  /// <summary>
  /// Pool of backend servers that actually serve requests
  /// </summary>
  virtual std::shared_ptr<ServiceBackend> getBackend(const std::string &ip) = 0;
  virtual std::vector<std::shared_ptr<ServiceBackend>> getBackendList() = 0;
  virtual void addBackend(const std::string &ip, const ServiceBackendJsonObject &conf) = 0;
  virtual void addBackendList(const std::vector<ServiceBackendJsonObject> &conf);
  virtual void replaceBackend(const std::string &ip, const ServiceBackendJsonObject &conf);
  virtual void replaceBackendList(const std::vector<ServiceBackendJsonObject> &conf) = 0;
  virtual void delBackend(const std::string &ip) = 0;
  virtual void delBackendList();

  std::shared_ptr<spdlog::logger> logger();
 protected:
  K8slbrp &parent_;
};