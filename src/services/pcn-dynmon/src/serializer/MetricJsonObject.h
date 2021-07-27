#pragma once

#include "JsonObjectBase.h"

namespace polycube {
  namespace service {
    namespace model {

      /**
       *  Metric
       */
      class MetricJsonObject : public JsonObjectBase {
       public:
        MetricJsonObject();
        MetricJsonObject(const nlohmann::json &json);
        ~MetricJsonObject() final = default;
        nlohmann::json toJson() const final;

        /**
         *  Name of the metric (e.g, number of HTTP requests)
         */
        std::string getName() const;
        void setName(std::string value);
        bool nameIsSet() const;

        /**
         *  Value of the metric
         */
        nlohmann::json getValue() const;
        void setValue(nlohmann::json value);
        bool valueIsSet() const;
        void unsetValue();

        /**
         *  Timestamp
         */
        int64_t getTimestamp() const;
        void setTimestamp(int64_t value);
        bool timestampIsSet() const;
        void unsetTimestamp();

       private:
        std::string m_name;
        bool m_nameIsSet;
        nlohmann::json m_value;
        bool m_valueIsSet;
        int64_t m_timestamp;
        bool m_timestampIsSet;
      };
    }// namespace model
  }// namespace service
}// namespace polycube
