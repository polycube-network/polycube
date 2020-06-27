#pragma once

#include "JsonObjectBase.h"
#include "PathConfigJsonObject.h"

namespace polycube {
  namespace service {
    namespace model {

      /**
       *  Dataplane Config
       */
      class DataplaneConfigJsonObject : public JsonObjectBase {
       public:
        DataplaneConfigJsonObject();
        DataplaneConfigJsonObject(const nlohmann::json &json);
        ~DataplaneConfigJsonObject() final = default;
        nlohmann::json toJson() const final;

        /**
         *  Dataplane configuration for the INGRESS path
         */
        PathConfigJsonObject getIngressPath() const;
        void setIngressPath(PathConfigJsonObject value);
        bool ingressPathIsSet() const;
        void unsetIngressPath();

        /**
         *  Dataplane configuration for the EGRESS path
         */
        PathConfigJsonObject getEgressPath() const;
        void setEgressPath(PathConfigJsonObject value);
        bool egressPathIsSet() const;
        void unsetEgressPath();

       private:
        PathConfigJsonObject m_ingressPath;
        bool m_ingressPathIsSet;
        PathConfigJsonObject m_egressPath;
        bool m_egressPathIsSet;
      };
    }// namespace model
  }// namespace service
}// namespace polycube
