/*
 * Copyright 2017 The Polycube Authors
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

// TODO: to be removed once pistache is fixed
#define PISTACHE_USE_SSL

#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include "polycube/services/json.hpp"
#include "polycubed_core.h"
#include "prometheus/counter.h"
#include "prometheus/exposer.h"
#include "prometheus/registry.h"
#include "prometheus/metric_family.h"
#include "prometheus/serializer.h"
#include "prometheus/text_serializer.h"
#include "prometheus/family.h"

// #define LOG_DEBUG_

#ifdef LOG_DEBUG_
#define LOG_DEBUG_REQUEST_
#define LOG_DEBUG_JSON_
#endif

using json = nlohmann::json;
using Pistache::Rest::Request;

namespace polycube {
namespace polycubed {

class RestServer {
 public:
  // RestServer(Net::Address addr);
  RestServer(Pistache::Address addr, PolycubedCore &core);

  std::shared_ptr<Pistache::Rest::Router> get_router();

  static const std::string base;

  void init(size_t thr = 1, const std::string &server_cert = "",
            const std::string &server_key = "",
            const std::string &root_ca_cert = "",
            const std::string &white_cert_list_path = "",
            const std::string &black_cert_list_path = "");

  void start();
  void shutdown();
  void load_last_topology();
  void create_metrics();


private:
  void setup_routes();
  // this function has to be static because has to be passed as a callback.
  static int client_verify_callback(int preverify_ok, void *ctx);

  void root_handler(const Pistache::Rest::Request &request,
                    Pistache::Http::ResponseWriter response);
  void root_help(HelpType type, Pistache::Http::ResponseWriter response);
  void root_completion(HelpType type, Pistache::Http::ResponseWriter response);

  void post_servicectrl(const Pistache::Rest::Request &request,
                        Pistache::Http::ResponseWriter response);
  void get_servicectrls(const Pistache::Rest::Request &request,
                        Pistache::Http::ResponseWriter response);
  void get_servicectrl(const Pistache::Rest::Request &request,
                       Pistache::Http::ResponseWriter response);
  void delete_servicectrl(const Pistache::Rest::Request &request,
                          Pistache::Http::ResponseWriter response);

  void get_cubes(const Pistache::Rest::Request &request,
                 Pistache::Http::ResponseWriter response);
  void get_cube(const Pistache::Rest::Request &request,
                Pistache::Http::ResponseWriter response);

  void post_cubes(const Pistache::Rest::Request &request,
                     Pistache::Http::ResponseWriter response);

  void cubes_help(const Pistache::Rest::Request &request,
                  Pistache::Http::ResponseWriter response);

  void cube_help(const Pistache::Rest::Request &request,
                 Pistache::Http::ResponseWriter response);

  void get_netdevs(const Pistache::Rest::Request &request,
                   Pistache::Http::ResponseWriter response);
  void get_netdev(const Pistache::Rest::Request &request,
                  Pistache::Http::ResponseWriter response);

  void netdevs_help(const Pistache::Rest::Request &request,
                    Pistache::Http::ResponseWriter response);

  void netdev_help(const Pistache::Rest::Request &request,
                   Pistache::Http::ResponseWriter response);

  void connect(const Pistache::Rest::Request &request,
               Pistache::Http::ResponseWriter response);
  void disconnect(const Pistache::Rest::Request &request,
                  Pistache::Http::ResponseWriter response);

  void connect_help(const Pistache::Rest::Request &request,
                    Pistache::Http::ResponseWriter response);
  void connect_completion(const std::string &p1,
                          Pistache::Http::ResponseWriter response);

  void attach(const Pistache::Rest::Request &request,
              Pistache::Http::ResponseWriter response);
  void detach(const Pistache::Rest::Request &request,
              Pistache::Http::ResponseWriter response);
  void topology(const Pistache::Rest::Request &request,
                Pistache::Http::ResponseWriter response);
  void get_if_topology(const Pistache::Rest::Request &request,
                Pistache::Http::ResponseWriter response);
  void topology_help(const Pistache::Rest::Request &request,
                     Pistache::Http::ResponseWriter response);

  void redirect(const Pistache::Rest::Request &request,
                Pistache::Http::ResponseWriter response);

  void get_version(const Pistache::Rest::Request &request,
                   Pistache::Http::ResponseWriter response);

  void create_cubes(json &cubes);

  void logRequest(const Pistache::Rest::Request &request);
  void logJson(json j);

  //metrics
  void get_metrics(const Pistache::Rest::Request &request,
                     Pistache::Http::ResponseWriter response);

  PolycubedCore &core;
  std::unique_ptr<Pistache::Http::Endpoint> httpEndpoint_;
  std::shared_ptr<Pistache::Rest::Router> router_;

  static std::string whitelist_cert_path;
  static std::string blacklist_cert_path;

  std::shared_ptr<spdlog::logger> logger;
  struct Metric {
        std::vector<std::reference_wrapper<prometheus::Family<prometheus::Counter>>> counters_family_;
        std::vector<std::reference_wrapper<prometheus::Counter>> counters_;
        std::vector<std::reference_wrapper<prometheus::Family<prometheus::Gauge>>> gauges_family_;
        std::vector<std::reference_wrapper<prometheus::Gauge>> gauges_;

    };
  std::map<std::string, Metric> map_metrics;
  std::vector<std::weak_ptr<prometheus::Collectable>> collectables_;
  std::shared_ptr<prometheus::Registry> registry;

};

}  // namespace polycubed
}  // namespace polycube
