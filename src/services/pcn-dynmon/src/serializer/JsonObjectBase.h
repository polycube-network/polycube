#pragma once
#include "polycube/services/fifo_map.hpp"
#include "polycube/services/json.hpp"
#include <ctime>
#include <string>

namespace polycube {
  namespace service {
    namespace model {

      class JsonObjectBase {
       public:
        JsonObjectBase() = default;
        JsonObjectBase(const nlohmann::json &base);
        virtual ~JsonObjectBase() = default;
        virtual nlohmann::json toJson() const = 0;
        static bool iequals(const std::string &a, const std::string &b);
        static std::string toJson(const std::string &value);
        static std::string toJson(const std::time_t &value);
        static int32_t toJson(int32_t value);
        static int64_t toJson(int64_t value);
        static double toJson(double value);
        static bool toJson(bool value);
        static nlohmann::json toJson(const JsonObjectBase &content);
        const nlohmann::json &getBase() const;
        void setBase(const nlohmann::json &base);

       private:
        nlohmann::json base_;
      };
    }// namespace model
  }// namespace service
}// namespace polycube
