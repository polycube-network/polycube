/*
 * Copyright 2018 The Polycube Authors
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

#include <spdlog/spdlog.h>
#include <string>

namespace configuration {

class Config {
 public:
  Config();
  ~Config();

  bool load(int argc, char *argv[]);
  void dump();

  // logging level used for polycubed, it is independent from each cube level
  spdlog::level::level_enum getLogLevel() const;
  void setLogLevel(const std::string &value);

  // if polycubed is run in daemon mode (detached from console)
  bool getDaemon() const;
  void setDaemon(const std::string &value);

  // pid file path
  std::string getPidFile() const;
  void setPidFile(const std::string &value);

  // port where the rest api listens
  uint16_t getServerPort() const;
  void setServerPort(const std::string &value);

  // server ip
  std::string getServerIP() const;
  void setServerIP(const std::string &value);

  // file where all logs are saved
  std::string getLogFile() const;
  void setLogFile(const std::string &value);

  // path of certificate & key to be used in server
  std::string getCertPath() const;
  void setCertPath(const std::string &value);
  std::string getKeyPath() const;
  void setKeyPath(const std::string &value);

  // path of root ca certificate used to authenticate clients
  std::string getCACertPath() const;
  void setCACertPath(const std::string &value);

  // white list certificates folder
  std::string getCertWhitelistPath() const;
  void setCertWhitelistPath(const std::string &value);

  // black list certificates folder
  std::string getCertBlacklistPath() const;
  void setCertBlacklistPath(const std::string &value);

 private:
  void load_from_file(const std::string &path);
  void load_from_cli(int argc, char *argv[]);
  void create_configuration_file(const std::string &path);
  // checks is the combinations of parameters is good
  void check();

  spdlog::level::level_enum loglevel;
  bool daemon;
  std::string pidfile;
  uint16_t server_port;
  std::string server_ip;
  std::string logfile;
  std::string configfile;
  std::string cert_path, key_path;
  std::string cacert_path;
  std::string cert_whitelist_path;
  std::string cert_blacklist_path;

  std::shared_ptr<spdlog::logger> logger;
};

extern Config config;
}
