#pragma once

#include "JsonObjectBase.h"

namespace polycube {
  namespace service {
    namespace model {
      /**
       *  Extraction Options
       */
      class ExtractionOptionsJsonObject : public JsonObjectBase {
       public:
        ExtractionOptionsJsonObject();
        ExtractionOptionsJsonObject(const nlohmann::json &json);
        ~ExtractionOptionsJsonObject() final = default;
        nlohmann::json toJson() const final;

        /**
         *  When true, map entries are deleted after being extracted
         */
        bool getEmptyOnRead() const;
        void setEmptyOnRead(bool value);
        bool emptyOnReadIsSet() const;
        void unsetEmptyOnRead();

        /**
         *  When true, the map is swapped with a new one before reading its content to provide thread safety and atomicity on read/write operations
         */
        bool getSwapOnRead() const;
        void setSwapOnRead(bool value);
        bool swapOnReadIsSet() const;
        void unsetSwapOnRead();

       private:
        bool m_swapOnRead;
        bool m_swapOnReadIsSet;
        bool m_emptyOnRead;
        bool m_emptyOnReadIsSet;
      };

    }// namespace model
  }// namespace service
}// namespace polycube
