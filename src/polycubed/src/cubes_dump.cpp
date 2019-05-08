/*
 * Copyright 2019 The Polycube Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fstream>
#include "cubes_dump.h"
#include "config.h"
#include "rest_server.h"
#include "server/Resources/Body/ListKey.h"
#include "server/Resources/Endpoint/Resource.h"

namespace polycube::polycubed {

using Rest::Resources::Body::ListKey;
using Rest::Resources::Endpoint::Operation;

CubesDump::CubesDump() : dump_enabled_(false), killSavingThread_(false) {
  save_in_file_thread_ = std::make_unique<std::thread>(&CubesDump::SaveToFile, this, configuration::config.getCubesDumpFilePath());
}

CubesDump::~CubesDump() {
  killSavingThread_ = true;
  waitForUpdate_.notify_one();
  save_in_file_thread_->join();
}

void CubesDump::Enable() {
  dump_enabled_ = true;
}

void CubesDump::UpdateCubesConfig(const std::string& resource,
                                  const nlohmann::json& body,
                                  const ListKeyValues &keys,
                                  Rest::Resources::Endpoint::Operation opType,
                                  Rest::Resources::Endpoint::ResourceType resType) {

  // TODO: remove this check
  if (configuration::config.getCubesNoDump()) {
    return;
  }

  std::string resourcePath = resource.substr(RestServer::base.size(), resource.size());

  std::stringstream ssResource(resourcePath);
  std::vector<std::string> resItem;
  std::string tokenResource;

  while (std::getline(ssResource, tokenResource, '/')) { /* Get path elements */
    resItem.push_back(tokenResource);
  }

  /*
   * Depending on the operation, we look into the resource string, the body
   * and the ListKeyValues to update the configuration of the cubes in the map
   * (<cubeName, cubeConfiguration>) we have in memory.
   * If it is not the initial topology load, the whole configuration is saved
   * to file after each modification by a separate thread, to avoid to overload
   * the server thread.
   */
  std::lock_guard<std::mutex> guardConfigMutex(cubesConfigMutex_);
  switch (opType) {
  case Operation::kCreate:
  case Operation::kReplace:
    UpdateCubesConfigCreateReplace(resItem, body, keys, resType);
    break;

  case Operation::kUpdate:
    UpdateCubesConfigUpdate(resItem, body, keys, resType);
    break;

  case Operation::kDelete:
    UpdateCubesConfigDelete(resItem, body, keys, resType);
    break;
  }

  /*
   * if the dump is enabled, notify the saving thread that an update
   * to the topology occured and needs to be dumped to file
   */
  if (dump_enabled_) {
    listOfPendingChanges_++;
    waitForUpdate_.notify_one();
  }
}

/*
 * This updates the configuration when the connect or disconnect special commands
 * are used, setting another cube's port as peer for a certain cube.
 */
void CubesDump::UpdatePortPeer(const std::string& cubeName, const std::string& cubePort, std::string peer) {

  // TODO: remove this check
  if (configuration::config.getCubesNoDump()) {
    return;
  }

  std::lock_guard<std::mutex> guardConfigMutex(cubesConfigMutex_);
  auto *cubePortsArray = &(cubesConfig_.at(cubeName).at("ports"));
  auto *portToConnect = &*(std::find_if(cubePortsArray->begin(), cubePortsArray->end(),
          [cubePort](auto const& elem){return elem.at("name") == cubePort;}));
  if (peer.empty()) {
    portToConnect->erase("peer");
  } else {
    nlohmann::json jsonPeer = nlohmann::json::object();
    jsonPeer["peer"] = peer;
    jsonPeer.update(*portToConnect);
    portToConnect->clear();
    portToConnect->update(jsonPeer);
  }

  /*
   * if the dump is enabled, notify the saving thread that an update
   * to the topology occured and needs to be dumped to file
   */
  if (dump_enabled_) {
    listOfPendingChanges_++;
    waitForUpdate_.notify_one();
  }
}

/*
 * This updates the configuration when the attach or detach special commands
 * are used, adding or removing transparent cubes from other cubes' ports.
 */
void CubesDump::UpdatePortTCubes(const std::string& cubeName,
                                 const std::string& cubePort,
                                 std::string tCubeName,
                                 int position) {

  // TODO: remove this check
  if (configuration::config.getCubesNoDump()) {
    return;
  }

  std::lock_guard<std::mutex> guardConfigMutex(cubesConfigMutex_);
  auto *cubePortsArray = &(cubesConfig_.at(cubeName).at("ports"));
  auto *interestedPort = &*(std::find_if(cubePortsArray->begin(), cubePortsArray->end(),
                                        [cubePort](auto const& elem){return elem.at("name") == cubePort;}));
  if (position == -1) {
    auto toDetach = std::find(interestedPort->at("tcubes").begin(), interestedPort->at("tcubes").end(), tCubeName);
    interestedPort->at("tcubes").erase(toDetach);
    if (interestedPort->at("tcubes").empty()) {
      interestedPort->erase("tcubes");
    }
  } else {
    nlohmann::json element; /* workaround to bypass nlohmann::json library bug */
    element.update(*interestedPort);
    if (element["tcubes"].is_null()) {
      std::vector<std::string> tCubesArray;
      tCubesArray.push_back(tCubeName);
      nlohmann::json completeObject = nlohmann::json::object();
      completeObject["tcubes"] = tCubesArray;
      completeObject.update(*interestedPort);
      interestedPort->clear();
      interestedPort->update(completeObject);
    } else {
      interestedPort->at("tcubes").insert(interestedPort->at("tcubes").begin() + position, tCubeName);
    }
  }

  /*
   * if the dump is enabled, notify the saving thread that an update
   * to the topology occured and needs to be dumped to file
   */
  if (dump_enabled_) {
    listOfPendingChanges_++;
    waitForUpdate_.notify_one();
  }
}

/*
 * This adds or replaces a cube or a cube's resource
 */
void CubesDump::UpdateCubesConfigCreateReplace(const std::vector<std::string> &resItem,
                                               const nlohmann::json& body,
                                               const ListKeyValues &keys,
                                               Rest::Resources::Endpoint::ResourceType resType) {

  const std::string &serviceName = resItem[0];
  const std::string &cubeName = resItem[1];

  // map saving key values corresponding to proper list name
  std::map<std::string, std::vector<Rest::Resources::Body::ListKeyValue>> keyValues;
  for (auto &key : keys) {
    keyValues[key.list_element].push_back(key);
  }

  switch (resType) {
    case Rest::Resources::Endpoint::ResourceType::Service: {
      nlohmann::json serviceField = nlohmann::json::object();
      serviceField["service-name"] = serviceName;
      serviceField.update(body);
      if (cubesConfig_.find(cubeName) != cubesConfig_.end()) { /* If it is a replacement, delete it first */
        cubesConfig_.at(cubeName).clear();
      }
      cubesConfig_[cubeName].update(serviceField);
      break;
    }

    case Rest::Resources::Endpoint::ResourceType::ParentResource:
    case Rest::Resources::Endpoint::ResourceType::ListResource: {
      nlohmann::json *item = &cubesConfig_[cubeName];
      for (int i = 2; i < resItem.size() - 1; i++) {
        nlohmann::json element; /* workaround to bypass nlohmann::json library bug */
        element.update(*item);
        auto *reference = &(*item)[resItem[i]];
        if (element[resItem[i]].is_null()) { /* is element present? */
          if (!keys.empty() && keyValues.find(resItem[i]) != keyValues.end()) { /* is it a missing array? */
            nlohmann::json toUpdate = nlohmann::json::array();
            toUpdate.push_back(body);
            nlohmann::json completeObj;
            completeObj[resItem[i]] = toUpdate;
            element.update(completeObj);
            *item = element;
            break;
          }
        } else if (element[resItem[i]].is_array()) { /* is it an array? */
          auto toCreateReplace = std::find_if(reference->begin(), reference->end(),
                  [resItem, i, keyValues, this](auto const& elem){return checkPresence(resItem, i, keyValues, elem);});
          i += keyValues.at(resItem[i]).size();
          if (i > resItem.size() - 2) { /* if it is an element of an array and it is the last, we can update it */
            if (toCreateReplace != reference->end()) {
              reference->erase(toCreateReplace);
            }
            reference->push_back(body);
          } else { /* not the last, find the element and update the reference */
            reference = &*toCreateReplace;
          }
        }
        item = reference;
      }
      break;
    }

    case Rest::Resources::Endpoint::ResourceType::LeafResource: {
      nlohmann::json *item = &cubesConfig_[cubeName];
      for (int i = 2; i < resItem.size() - 1; i++) {
        auto *reference = &(*item)[resItem[i]];
        if (reference->is_array()) { /* find the array element and update the reference */
          reference = &*(std::find_if(reference->begin(), reference->end(),
                  [resItem, i, keyValues, this](auto const& elem){return checkPresence(resItem, i, keyValues, elem);}));
          i += keyValues.at(resItem[i]).size();
        } else if (i > resItem.size() - 2) {
          nlohmann::json toUpdate = nlohmann::json::object();
          toUpdate[resItem[resItem.size() - 1]] = body;
          toUpdate.insert(item->begin(), item->end());
          item->clear();
          item->update(toUpdate);
        }
        item = reference;
      }
      break;
    }
  }
}

/*
 * This updates a cube or a cube's resource.
 */
void CubesDump::UpdateCubesConfigUpdate(const std::vector<std::string> &resItem,
                                        const nlohmann::json body,
                                        const ListKeyValues &keys,
                                        Rest::Resources::Endpoint::ResourceType resType) {

  const std::string &serviceName = resItem[0];
  const std::string &cubeName = resItem[1];

  std::map<std::string, std::vector<Rest::Resources::Body::ListKeyValue>> keyValues;
  for (auto &key : keys) {
    keyValues[key.list_element].push_back(key);
  }

  if (resType == Rest::Resources::Endpoint::ResourceType::Service) {
    nlohmann::json serviceField = nlohmann::json::object();
    serviceField.update(body);
    cubesConfig_.at(cubeName).update(serviceField);
  } else {
    nlohmann::json *item = &cubesConfig_[cubeName];
    for (int i = 2; i < resItem.size() - 1; i++) {
      auto *reference = &(*item)[resItem[i]];
      if (reference->is_array()) { /* If it is an array corresponding to a key, then it is an element of an array */
        reference = &*(std::find_if(reference->begin(), reference->end(),
                [resItem, i, keyValues, this](auto const& elem){return checkPresence(resItem, i, keyValues, elem);}));
        i += keyValues.at(resItem[i]).size();
      }
      item = reference;
    }

    nlohmann::json toUpdate = nlohmann::json::object();
    toUpdate[resItem[resItem.size() - 1]] = body;
    toUpdate.insert(item->begin(), item->end());
    item->clear();
    item->update(toUpdate);
  }
}

/*
 * This deletes a cube or a cube's resource.
 */
void CubesDump::UpdateCubesConfigDelete(const std::vector<std::string> &resItem,
                                        const nlohmann::json& body,
                                        const ListKeyValues &keys,
                                        Rest::Resources::Endpoint::ResourceType resType) {

  const std::string &serviceName = resItem[0];
  const std::string &cubeName = resItem[1];

  std::map<std::string, std::vector<Rest::Resources::Body::ListKeyValue>> keyValues;
  for (auto &key : keys) {
    keyValues[key.list_element].push_back(key);
  }

  if (resType == Rest::Resources::Endpoint::ResourceType::Service) { /* If it regards the full service */
    cubesConfig_.erase(cubeName);
  } else { /* If it regards an element of that service */
    nlohmann::json *item = &cubesConfig_[cubeName];
    bool deleted = false;
    for (int i = 2; i < resItem.size() - 1; i++) {
      auto *reference = &(*item)[resItem[i]];
      if (reference->is_array()) { /* If it is an element of an array, treat it differently */
        int actualIndex = i;
        auto toDelete = std::find_if(reference->begin(), reference->end(),
                [resItem, i, keyValues, this](auto const& elem){return checkPresence(resItem, i, keyValues, elem);});
        i += keyValues.at(resItem[i]).size();
        if (i > resItem.size() - 2) { /* if it is an array element, delete it now and set deleted true */
          reference->erase(toDelete);
          deleted = true;
          if (reference->empty()) { /* delete if array is empty */
            item->erase(resItem[actualIndex]);
          }
        } else { /* update the reference to the array elem */
          reference = &*toDelete;
        }
      }
      item = reference;
    }

    if (!deleted) {
      item->erase(resItem[resItem.size() - 1]);
    }
  }
}

/*
 * This function is called in a thread when the cube-dump option is enabled
 */
void CubesDump::SaveToFile(const std::string& path) {
  while (true) {
    // mutex with condition variable waitForUpdate on cubesConfig_
    std::unique_lock<std::mutex> cubesConfigLock(cubesConfigMutex_);
    // retrieve the value of the atomic variable and if there are no updates from last write to file,
    // either wait for updates (kill=false) or exit if the daemon is shutting down (kill=true)
    if (listOfPendingChanges_.load() == 0 && !killSavingThread_) {
      waitForUpdate_.wait(cubesConfigLock);
    }

    if (listOfPendingChanges_.load() == 0 && killSavingThread_) {
      break;
    }

    std::map<std::string, nlohmann::json> copyConfig(cubesConfig_);
    // reset to 0 the list of pending changes
    listOfPendingChanges_.store(0);
    cubesConfigLock.unlock();
    std::ofstream cubesDump(path);

    if (cubesDump.is_open()) {
      nlohmann::json toDump = nlohmann::json::array();
      for (const auto &elem : copyConfig) {
        auto cube = elem.second;
        toDump += cube;
      }
      cubesDump << toDump.dump(2);
      cubesDump.close();
    }
  }
}

/*
 * checks if an element with certain key values exists
 */
bool CubesDump::checkPresence(const std::vector<std::string> &resItem,
                              const int &resItemIndex,
                              const std::map<std::string, std::vector<Rest::Resources::Body::ListKeyValue>> &keyValues,
                              const nlohmann::json &elem) {
  int valuesNumber = keyValues.at(resItem[resItemIndex]).size();
  int index = 0;

  for (auto &element : keyValues.at(resItem[resItemIndex])) {
    if (std::string(elem[keyValues.at(resItem[resItemIndex])[index].original_key]) == resItem[resItemIndex + index + 1]) {
      valuesNumber--;
      index++;
    }
  }

  return valuesNumber == 0;
}

} // namespace polycube::polycubed