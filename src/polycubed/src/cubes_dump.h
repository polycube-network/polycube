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

#pragma once

#include <string>
#include <polycube/services/json.hpp>
#include <server/Resources/Body/ListKey.h>
#include <server/Resources/Endpoint/Resource.h>

namespace polycube::polycubed {

class CubesDump {
 public:
  CubesDump();
  ~CubesDump();

  void Enable();

  void UpdateCubesConfig(const std::string &resource,
                         const nlohmann::json &body,
                         const ListKeyValues &keys,
                         Rest::Resources::Endpoint::Operation opType,
                         Rest::Resources::Endpoint::ResourceType resType);

  void UpdatePortPeer(const std::string& cubeName, const std::string& cubePort, std::string peer);

  void UpdatePortTCubes(const std::string& cubeName, const std::string& cubePort, std::string tCubeName, int position);

 private:
  void UpdateCubesConfigCreateReplace(const std::vector<std::string> &resItem,
                                      const nlohmann::json &body,
                                      const ListKeyValues &keys,
                                      Rest::Resources::Endpoint::ResourceType resType);

  void UpdateCubesConfigUpdate(const std::vector<std::string> &resItem,
                               const nlohmann::json body,
                               const ListKeyValues &keys,
                               Rest::Resources::Endpoint::ResourceType resType);

  void UpdateCubesConfigDelete(const std::vector<std::string> &resItem,
                               const nlohmann::json &body,
                               const ListKeyValues &keys,
                               Rest::Resources::Endpoint::ResourceType resType);

  bool checkPresence(const std::vector<std::string> &resItem,
                     const int &resItemIndex,
                     const std::map<std::string, std::vector<Rest::Resources::Body::ListKeyValue>> &keyValues,
                     const nlohmann::json &elem);

  void SaveToFile(const std::string &path);

  // thread that saves in file
  std::unique_ptr<std::thread> save_in_file_thread_;
  // mutex on the access to cubesConfig
  std::mutex cubesConfigMutex_;
  // cubes configuration <name, configuration>
  std::map<std::string, nlohmann::json> cubesConfig_;
  // how many updates have been made while thread saving to file.
  // it is incremented by the server to notify when an update is available while
  // the saving thread was not waiting on the condition_variable
  std::atomic<int> listOfPendingChanges_;
  // wait until an update occurs
  std::condition_variable waitForUpdate_;
  // the saving thread ends if the daemon is shutting down (kill=true)
  bool killSavingThread_;
  bool dump_enabled_;
};
} // namespace polycube::polycubed