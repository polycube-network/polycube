#pragma once

#include "JsonObjectBase.h"

namespace polycube {
  namespace service {
    namespace model {

      /**
       *  Label
       */
      class LabelJsonObject : public JsonObjectBase {
       public:
        LabelJsonObject();
        LabelJsonObject(const nlohmann::json &json);
        ~LabelJsonObject() final = default;
        nlohmann::json toJson() const final;

        /**
         *  Name of the label (e.g., &#39;method&#39;)
         */
        std::string getName() const;
        void setName(std::string value);
        bool nameIsSet() const;

        /**
         *  Label value (e.g., &#39;POST&#39;)
         */
        std::string getValue() const;
        void setValue(std::string value);
        bool valueIsSet() const;

       private:
        std::string m_name;
        bool m_nameIsSet;
        std::string m_value;
        bool m_valueIsSet;
      };
    }// namespace model
  }// namespace service
}// namespace polycube
