/**
* nat API
* nat API generated from nat.yang
*
* OpenAPI spec version: 1.0.0
*
* NOTE: This class is auto generated by the swagger code generator program.
* https://github.com/polycube-network/swagger-codegen.git
* branch polycube
*/


/* Do not edit this file manually */

/*
* RuleSnatJsonObject.h
*
*
*/

#pragma once


#include "JsonObjectBase.h"

#include <vector>
#include "RuleSnatEntryJsonObject.h"

namespace io {
namespace swagger {
namespace server {
namespace model {


/// <summary>
///
/// </summary>
class  RuleSnatJsonObject : public JsonObjectBase {
public:
  RuleSnatJsonObject();
  RuleSnatJsonObject(const nlohmann::json &json);
  ~RuleSnatJsonObject() final = default;
  nlohmann::json toJson() const final;


  /// <summary>
  /// List of Source NAT rules
  /// </summary>
  const std::vector<RuleSnatEntryJsonObject>& getEntry() const;
  void addRuleSnatEntry(RuleSnatEntryJsonObject value);
  bool entryIsSet() const;
  void unsetEntry();

private:
  std::vector<RuleSnatEntryJsonObject> m_entry;
  bool m_entryIsSet;
};

}
}
}
}

