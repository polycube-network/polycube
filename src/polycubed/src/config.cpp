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

#include "config.h"
#include <getopt.h>
#include <linux/limits.h>
#include <string.h>
#include <sys/stat.h>
#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>

#include "version.h"

namespace configuration {

#define LOGLEVEL spdlog::level::level_enum::info
#define DAEMON false
#define PIDFILE "/var/run/polycube.pid"
#define SERVER_PORT 9000
#define SERVER_IP "localhost"
#define LOGFILE "/var/log/polycube/polycubed.log"
#define CONFIGFILEDIR "/etc/polycube"
#define CONFIGFILENAME "polycubed.conf"
#define CONFIGFILE (CONFIGFILEDIR "/" CONFIGFILENAME)

static void show_usage(const std::string &name) {
  std::cout << std::boolalpha;
  std::cout << "Usage: " << name << " [OPTIONS]" << std::endl;
  std::cout << "-p, --port PORT: port where the rest server listens (default: "
            << SERVER_PORT << ")" << std::endl;
  std::cout << "-a, --addr: addr where polycubed listens (default: "
            << SERVER_IP << ")" << std::endl;
  std::cout << "-l, --loglevel: set log level (trace, debug, info, warn, err, "
               "critical, off, default: info)"
            << std::endl;
  std::cout << "-c, --cert: path to ssl certificate" << std::endl;
  std::cout << "-k, --key: path to ssl private key" << std::endl;
  std::cout << "--cacert: path to certification authority certificate used to "
               "validate clients"
            << std::endl;
  std::cout << "-d, --daemon: run as daemon (default: " << DAEMON << ")"
            << std::endl;
  std::cout << "-v, --version: show version and exit" << std::endl;
  std::cout << "--logfile: file to save polycube logs (default: " << LOGFILE
            << ")" << std::endl;
  std::cout << "--pidfile: file to save polycubed pid (default: " << PIDFILE
            << ")" << std::endl;
  std::cout << "--configfile: configuration file (default: " << CONFIGFILE
            << ")" << std::endl;
  std::cout << "--cert-blacklist: path to black listed certificates"
            << std::endl;
  std::cout << "--cert-whitelist: path to white listed certificates"
            << std::endl;
  std::cout << "-h, --help: print this message" << std::endl;
}

static void show_version(void) {
  std::cout << "polycubed version " << VERSION << std::endl;
}

bool parse_bool(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  if (str == "true") {
    return true;
  } else if (str == "false") {
    return false;
  } else {
    throw std::runtime_error(str + " is not a boolean");
  }
}

Config config;

Config::Config()
    : loglevel(LOGLEVEL),
      daemon(DAEMON),
      pidfile(PIDFILE),
      server_port(SERVER_PORT),
      server_ip(SERVER_IP),
      logfile(LOGFILE),
      configfile(CONFIGFILE) {}

Config::~Config() {}

spdlog::level::level_enum Config::getLogLevel() const {
  return loglevel;
}

#define CHECK_OVERWRITE(PARAMETER, NEW_VALUE, CURRENT_VALUE, DEFAULT)        \
  do {                                                                       \
    if (NEW_VALUE == CURRENT_VALUE) {                                        \
      return;                                                                \
    }                                                                        \
    if (CURRENT_VALUE != DEFAULT) {                                          \
      logger->warn("'--" PARAMETER                                           \
                   "' also present on configuration file, overwritting it"); \
    }                                                                        \
  } while (0)

void Config::setLogLevel(const std::string &value_) {
  std::string value(value_);
  std::transform(value.begin(), value.end(), value.begin(), ::tolower);
  spdlog::level::level_enum level_ = spdlog::level::from_str(value);
  // from_str returns off if the level is not valid, be more agressive on that
  // check
  if (level_ == spdlog::level::level_enum::off && value != "off") {
    throw std::runtime_error(value + " is not a valid logging level");
  }

  CHECK_OVERWRITE("logevel", level_, loglevel, LOGLEVEL);
  loglevel = level_;
}

bool Config::getDaemon() const {
  return daemon;
}

void Config::setDaemon(const std::string &value) {
  bool daemon_ = parse_bool(value);
  CHECK_OVERWRITE("daemon", daemon_, daemon, DAEMON);
  daemon = daemon_;
}

std::string Config::getPidFile() const {
  return pidfile;
}

void Config::setPidFile(const std::string &value) {
  CHECK_OVERWRITE("pidfile", value, pidfile, PIDFILE);
  pidfile = value;
}

uint16_t Config::getServerPort() const {
  return server_port;
}

void Config::setServerPort(const std::string &value) {
  unsigned long port_ = std::atol(optarg);
  if (port_ > UINT16_MAX) {
    throw std::runtime_error("port value" + std::string(optarg) +
                             " is not not valid");
  }

  CHECK_OVERWRITE("port", port_, server_port, SERVER_PORT);
  server_port = port_;
}

std::string Config::getServerIP() const {
  return server_ip;
}

void Config::setServerIP(const std::string &value) {
  CHECK_OVERWRITE("addr", value, server_ip, SERVER_IP);
  server_ip = value;
}

std::string Config::getLogFile() const {
  return logfile;
}

void Config::setLogFile(const std::string &value) {
  CHECK_OVERWRITE("logfile", value, logfile, LOGFILE);
  logfile = value;
}

std::string Config::getCertPath() const {
  return cert_path;
}

void Config::setCertPath(const std::string &value) {
  CHECK_OVERWRITE("cert", value, cert_path, "");
  cert_path = value;
}

std::string Config::getKeyPath() const {
  return key_path;
}

void Config::setKeyPath(const std::string &value) {
  CHECK_OVERWRITE("key", value, key_path, "");
  key_path = value;
}

std::string Config::getCACertPath() const {
  return cacert_path;
}

void Config::setCACertPath(const std::string &value) {
  CHECK_OVERWRITE("cacert", value, cacert_path, "");
  cacert_path = value;
}

std::string Config::getCertWhitelistPath() const {
  return cert_whitelist_path;
}

void Config::setCertWhitelistPath(const std::string &value) {
  CHECK_OVERWRITE("cert-whitelist", value, cert_whitelist_path, "");
  cert_whitelist_path = value;
}

std::string Config::getCertBlacklistPath() const {
  return cert_blacklist_path;
}

void Config::setCertBlacklistPath(const std::string &value) {
  CHECK_OVERWRITE("cert-blacklist", value, cert_blacklist_path, "");
  cert_blacklist_path = value;
}

void Config::create_configuration_file(const std::string &path) {
  mkdir(CONFIGFILEDIR, 0600);
  std::ofstream file(path);
  if (!file.good()) {
    throw std::runtime_error("error creating configuration file");
  }
  file << std::boolalpha;  // print booleans as "true"/"false" instead of "1/0"
  file << "# polycubed configuration file" << std::endl << std::endl;
  file << "# logging level (trace, debug, info, warn, err, critical, off)"
       << std::endl;
  file << "loglevel: " << spdlog::level::to_c_str(loglevel) << std::endl;
  file << "# run as daemon" << std::endl;
  file << "daemon: " << daemon << std::endl;
  file << "# file to save polycubed pid" << std::endl;
  file << "pidfile: " << pidfile << std::endl;
  file << "# port where the rest server listens" << std::endl;
  file << "port: " << server_port << std::endl;
  file << "# addr where polycubed listens" << std::endl;
  file << "addr: " << server_ip << std::endl;
  file << "# file to save polycube logs" << std::endl;
  file << "logfile: " << logfile << std::endl;
  file << "# Security related:" << std::endl;
  file << "# server certificate " << std::endl;
  file << "#cert: path_to_certificate_file" << std::endl;
  file << "# server private key " << std::endl;
  file << "#key: path_to_key_fule" << std::endl;
  file << "# certification authority certificate " << std::endl;
  file << "#cacert: path_to_certificate_file" << std::endl;
  file << "# path to black list certificates folder" << std::endl;
  file << "#cert-blacklist: path to folder containing black listed certificates"
       << std::endl;
  file << "# path to white list certificates folder" << std::endl;
  file << "#cert-whitelist: path to folder containing white listed certificates"
       << std::endl;
}

void Config::dump() {
  logger = spdlog::get("polycubed");
  logger->info("configuration parameters: ");
  logger->info(" loglevel: {}", spdlog::level::to_c_str(loglevel));
  logger->info(" daemon: {}", daemon);
  logger->info(" pidfile: {}", pidfile);
  logger->info(" port: {}", server_port);
  logger->info(" addr: {}", server_ip);
  logger->info(" logfile: {}", logfile);
  if (!cert_path.empty()) {
    logger->info(" cert: {}", cert_path);
  }
  if (!key_path.empty()) {
    logger->info(" key: {}", key_path);
  }
  if (!cacert_path.empty()) {
    logger->info(" cacert: {}", cacert_path);
  }
  if (!cert_blacklist_path.empty()) {
    logger->info(" blacklist: {}", cert_blacklist_path);
  }
  if (!cert_whitelist_path.empty()) {
    logger->info(" whitelist: {}", cert_whitelist_path);
  }
}

void Config::load_from_file(const std::string &path) {
  logger->info("loading configuration from {}", path);
  std::ifstream file(path);
  if (!file.good()) {
    if (path == CONFIGFILE) {
      logger->warn(
          "default configfile ({}) not found, creating a new with default "
          "parameters",
          CONFIGFILE);
      create_configuration_file(CONFIGFILE);
    } else {
      throw std::runtime_error("error opening " + path);
    }
  }
  std::string str;
  std::string value;
  size_t pos;

  char *argv[50];
  argv[0] = strndup("", PATH_MAX);
  int argc = 1;

  std::smatch match;
  std::regex rule("(\\S+):(\\S+)");

  int i = 0;
  while (std::getline(file, str)) {
    i++;
    // remove all spaces
    str.erase(remove_if(str.begin(), str.end(), isspace), str.end());

    // remove comments
    size_t pos = str.find('#');
    if (pos != std::string::npos) {
      str.erase(pos);
    }

    if (str.empty()) {
      continue;
    }

    if (!std::regex_match(str, match, rule)) {
      std::stringstream err;
      err << "bad line in configuration file: " << path << ":" << i << " -> \""
          << str << "\"" << std::endl;
      throw std::runtime_error(err.str().c_str());
    }

    std::replace(str.begin(), str.end(), ':', '=');

    value = "--" + str;
    argv[argc++] = strndup(value.c_str(), PATH_MAX);
  }

  load_from_cli(argc, argv);

  for (int i = 0; i < argc; i++) {
    free(argv[i]);
  }
}

static struct option options[] = {
    {"loglevel", required_argument, NULL, 'l'},
    {"port", required_argument, NULL, 'p'},
    {"daemon", optional_argument, NULL, 'd'},
    {"addr", required_argument, NULL, 'a'},
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'v'},
    {"cert", required_argument, NULL, 'c'},
    {"key", required_argument, NULL, 'k'},
    {"logfile", required_argument, NULL, 1},
    {"pidfile", required_argument, NULL, 2},
    {"configfile", required_argument, NULL, 4},
    {"cacert", required_argument, NULL, 5},
    {"cert-whitelist", required_argument, NULL, 6},
    {"cert-blacklist", required_argument, NULL, 7},
    {NULL, 0, NULL, 0},
};

void Config::load_from_cli(int argc, char *argv[]) {
  int option_index = 0;
  char ch;
  optind = 0;
  while ((ch = getopt_long(argc, argv, "l:p:a:dhv", options, &option_index)) !=
         -1) {
    switch (ch) {
    case 'l':
      setLogLevel(optarg);
      break;
    case 'p':
      setServerPort(optarg);
      break;
    case 'd':
      setDaemon(optarg ? std::string(optarg) : "true");
      break;
    case 'a':
      setServerIP(optarg);
      break;
    case 'c':
      setCertPath(optarg);
      break;
    case 'k':
      setKeyPath(optarg);
      break;
    case '?':
      throw std::runtime_error("Missing argument, see stderr");
    case 1:
      setLogFile(optarg);
      break;
    case 2:
      setPidFile(optarg);
      break;
    case 5:
      setCACertPath(optarg);
      break;
    case 6:
      setCertWhitelistPath(optarg);
      break;
    case 7:
      setCertBlacklistPath(optarg);
      break;
    }
  }
}

bool Config::load(int argc, char *argv[]) {
  logger = spdlog::get("polycubed");

  int option_index = 0;
  char ch;

  // do a first pass looking for "configfile", "-h", "-v"
  while ((ch = getopt_long(argc, argv, "l:p:a:dhv", options, &option_index)) !=
         -1) {
    switch (ch) {
    case 'v':
      show_version();
      return false;
    case 'h':
      show_usage(argv[0]);
      return false;
    case 4:
      configfile = optarg;
      break;
    }
  }

  load_from_file(configfile);
  load_from_cli(argc, argv);
  check();
  return true;
}

void Config::check() {
  // cert and key has to both to be passed
  if (cert_path != "" && key_path == "") {
    throw std::runtime_error("-c, --cert requires -k, --key");
  }

  if (cert_path == "" && key_path != "") {
    throw std::runtime_error("-k, --key requires -c, --cert");
  }

  if (cert_path != "" && cacert_path == "" && cert_whitelist_path == "") {
    throw std::runtime_error(
        "--cacert or --cert-whitelist are needed when using --cert");
  }

  if (cacert_path != "" && cert_path == "") {
    throw std::runtime_error("--cacert requires -c, --cert and -k, --key");
  }

  if (cacert_path != "" && cert_whitelist_path != "") {
    throw std::runtime_error(
        "--cacert and --cert-whitelist cannot be used together");
  }

  if (cert_whitelist_path != "" && cert_blacklist_path != "") {
    throw std::runtime_error(
        "--cert-blacklist & --cert-whitelist cannot be used together");
  }

  if (cert_blacklist_path != "" && cacert_path == "") {
    throw std::runtime_error("--cert-blacklist requires --cacert");
  }
}
}
