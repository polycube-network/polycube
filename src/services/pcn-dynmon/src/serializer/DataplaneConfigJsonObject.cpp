#include "DataplaneConfigJsonObject.h"
#include <regex>

namespace polycube {
  namespace service {
    namespace model {
      DataplaneConfigJsonObject::DataplaneConfigJsonObject() {
        m_ingressPathIsSet = false;
        m_egressPathIsSet = false;
      }

      DataplaneConfigJsonObject::DataplaneConfigJsonObject(const nlohmann::json &val) : JsonObjectBase(val) {
        m_ingressPathIsSet = false;
        m_egressPathIsSet = false;

        if (val.count("ingress-path"))
          if (!val["ingress-path"].is_null()) {
            PathConfigJsonObject newItem{val["ingress-path"]};
            setIngressPath(newItem);
          }
        if (val.count("egress-path"))
          if (!val["egress-path"].is_null()) {
            PathConfigJsonObject newItem{val["egress-path"]};
            setEgressPath(newItem);
          }
      }

      nlohmann::json DataplaneConfigJsonObject::toJson() const {
        nlohmann::json val = nlohmann::json::object();
        if (!getBase().is_null())
          val.update(getBase());
        if (m_ingressPathIsSet)
          val["ingress-path"] = JsonObjectBase::toJson(m_ingressPath);
        if (m_egressPathIsSet)
          val["egress-path"] = JsonObjectBase::toJson(m_egressPath);
        return val;
      }

      PathConfigJsonObject DataplaneConfigJsonObject::getIngressPath() const {
        return m_ingressPath;
      }

      void DataplaneConfigJsonObject::setIngressPath(PathConfigJsonObject value) {
        m_ingressPath = value;
        m_ingressPathIsSet = true;
      }

      bool DataplaneConfigJsonObject::ingressPathIsSet() const {
        return m_ingressPathIsSet;
      }

      void DataplaneConfigJsonObject::unsetIngressPath() {
        m_ingressPathIsSet = false;
      }

      PathConfigJsonObject DataplaneConfigJsonObject::getEgressPath() const {
        return m_egressPath;
      }

      void DataplaneConfigJsonObject::setEgressPath(PathConfigJsonObject value) {
        m_egressPath = value;
        m_egressPathIsSet = true;
      }

      bool DataplaneConfigJsonObject::egressPathIsSet() const {
        return m_egressPathIsSet;
      }

      void DataplaneConfigJsonObject::unsetEgressPath() {
        m_egressPathIsSet = false;
      }
    }// namespace model
  }// namespace service
}// namespace polycube
