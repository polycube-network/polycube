#include "ExtractionOptionsJsonObject.h"
#include <regex>

namespace polycube {
  namespace service {
    namespace model {

      ExtractionOptionsJsonObject::ExtractionOptionsJsonObject() {
        m_swapOnRead = false;
        m_swapOnReadIsSet = true;
        m_emptyOnRead = false;
        m_emptyOnReadIsSet = true;
      }

      ExtractionOptionsJsonObject::ExtractionOptionsJsonObject(const nlohmann::json &val) : JsonObjectBase(val) {
        m_swapOnReadIsSet = false;
        m_emptyOnReadIsSet = false;
        if (val.count("swap-on-read"))
          setSwapOnRead(val.at("swap-on-read").get<bool>());
        if (val.count("empty-on-read"))
          setEmptyOnRead(val.at("empty-on-read").get<bool>());
      }

      nlohmann::json ExtractionOptionsJsonObject::toJson() const {
        nlohmann::json val = nlohmann::json::object();
        if (!getBase().is_null())
          val.update(getBase());
        if (m_swapOnReadIsSet)
          val["swap-on-read"] = m_swapOnRead;
        if (m_emptyOnReadIsSet)
          val["empty-on-read"] = m_emptyOnRead;
        return val;
      }

      bool ExtractionOptionsJsonObject::getSwapOnRead() const {
        return m_swapOnRead;
      }

      void ExtractionOptionsJsonObject::setSwapOnRead(bool value) {
        m_swapOnRead = value;
        m_swapOnReadIsSet = true;
      }

      bool ExtractionOptionsJsonObject::swapOnReadIsSet() const {
        return m_swapOnReadIsSet;
      }

      void ExtractionOptionsJsonObject::unsetSwapOnRead() {
        m_swapOnReadIsSet = false;
      }

      bool ExtractionOptionsJsonObject::getEmptyOnRead() const {
        return m_emptyOnRead;
      }

      void ExtractionOptionsJsonObject::setEmptyOnRead(bool value) {
        m_emptyOnRead = value;
        m_emptyOnReadIsSet = true;
      }

      bool ExtractionOptionsJsonObject::emptyOnReadIsSet() const {
        return m_emptyOnReadIsSet;
      }

      void ExtractionOptionsJsonObject::unsetEmptyOnRead() {
        m_emptyOnReadIsSet = false;
      }
    }// namespace model
  }// namespace service
}// namespace polycube
