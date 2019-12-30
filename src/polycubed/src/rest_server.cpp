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
#include "rest_server.h"

#include <string>
#include <vector>
#include <fstream>


#include "polycubed_core.h"
#include "service_controller.h"
#include "version.h"
#include "utils/netlink.h"

#include "polycube/services/response.h"
#include "server/Resources/Data/AbstractFactory.h"
#include "server/Server/ResponseGenerator.h"
#include "config.h"
#include "cubes_dump.h"
#include "jsoncons/json.hpp"
#include "jsoncons_ext/jsonpath/json_query.hpp"
#include <typeinfo>

namespace polycube {
    namespace polycubed {

        std::string RestServer::whitelist_cert_path;
        std::string RestServer::blacklist_cert_path;

        const std::string RestServer::base = "/polycube/v1/";

// start http server for Management APIs
// Incapsultate a core object // TODO probably there are best ways...
        RestServer::RestServer(Pistache::Address addr, PolycubedCore &core)
                : core(core),
                  httpEndpoint_(std::make_unique<Pistache::Http::Endpoint>(addr)),
                  logger(spdlog::get("polycubed")) {
            logger->info("rest server listening on '{0}:{1}'", addr.host(), addr.port());
            router_ = std::make_shared<Pistache::Rest::Router>();
        }

        std::shared_ptr<Pistache::Rest::Router> RestServer::get_router() {
            return router_;
        }

        static X509_STORE_CTX *load_certificates(const char *path) {
            X509_STORE *store;
            X509_STORE_CTX *ctx;

            store = X509_STORE_new();
            X509_STORE_load_locations(store, nullptr, path);

            ctx = X509_STORE_CTX_new();
            X509_STORE_CTX_init(ctx, store, nullptr, nullptr);

            return ctx;
        }

#ifndef X509_STORE_get1_cert
#define X509_STORE_get1_cert X509_STORE_get1_certs
#endif

        static bool lookup_cert_match(X509_STORE_CTX *ctx, X509 *x) {
            STACK_OF(X509) * certs;
            X509 *xtmp = nullptr;
            int i;
            bool found = false;
            /* Lookup all certs with matching subject name */
            certs = X509_STORE_get1_cert(ctx, X509_get_subject_name(x));
            if (certs == nullptr)
                return found;
            /* Look for exact match */
            for (i = 0; i < sk_X509_num(certs); i++) {
                xtmp = sk_X509_value(certs, i);
                if (!X509_cmp(xtmp, x)) {
                    found = true;
                    break;
                }
            }
            sk_X509_pop_free(certs, X509_free);
            return found;
        }

        int RestServer::client_verify_callback(int preverify_ok, void *ctx) {
            X509_STORE_CTX *x509_ctx = (X509_STORE_CTX *) ctx;
            int depth = X509_STORE_CTX_get_error_depth(x509_ctx);
            if (depth == 0) {
                X509 *cert = X509_STORE_CTX_get_current_cert(x509_ctx);
                X509_STORE_CTX *list;

                // if certificate is on white list we don't care about preverify
                if (!RestServer::whitelist_cert_path.empty()) {
                    list = load_certificates(RestServer::whitelist_cert_path.c_str());
                    return lookup_cert_match(list, cert);
                }

                // at this point certificate should also pass ca verification, test it
                if (!preverify_ok)
                    return preverify_ok;

                // test that the certificate is not on a black list
                if (!RestServer::blacklist_cert_path.empty()) {
                    list = load_certificates(RestServer::blacklist_cert_path.c_str());
                    return !lookup_cert_match(list, cert);
                }
            }
            return preverify_ok;
        }

        void RestServer::init(size_t thr, const std::string &server_cert,
                              const std::string &server_key,
                              const std::string &root_ca_cert,
                              const std::string &whitelist_cert_path_,
                              const std::string &blacklist_cert_path_) {
            logger->debug("rest server will use {0} thread(s)", thr);
            auto opts = Pistache::Http::Endpoint::options().threads(thr).flags(
                    Pistache::Tcp::Options::InstallSignalHandler |
                    Pistache::Tcp::Options::ReuseAddr);
            httpEndpoint_->init(opts);

            if (!server_cert.empty()) {
                httpEndpoint_->useSSL(server_cert, server_key);
            }

            if (!root_ca_cert.empty() || !whitelist_cert_path_.empty() ||
                !blacklist_cert_path_.empty()) {
                httpEndpoint_->useSSLAuth(root_ca_cert, "", client_verify_callback);
            }

            RestServer::whitelist_cert_path = whitelist_cert_path_;
            RestServer::blacklist_cert_path = blacklist_cert_path_;

            setup_routes();
        }

        void RestServer::start() {
            logger->info("rest server starting ...");
            httpEndpoint_->setHandler(Pistache::Rest::Router::handler(router_));
            httpEndpoint_->serveThreaded();
        }


        void RestServer::shutdown() {
            logger->info("shutting down rest server ...");
            try {
                httpEndpoint_->shutdown();
            } catch (const std::runtime_error &e) {
                logger->error("{0}", e.what());
            }
        }

        void RestServer::create_cubes(json &cubes) {
            /*
             * This is possible that a cube's port has as peer another port
             * of a cube that hasn't been created yet.  This logic removes all
             * the peers from the request and sets them after all cubes have
             * been created.
             */
            auto peers = utils::strip_port_peers(cubes);
            std::vector<Response> resp = {{ErrorTag::kNoContent, nullptr}};
            bool error = false;

            for (auto &cube : cubes) {
                auto &s = core.get_service_controller(cube["service-name"]);
                auto res = s.get_management_interface()->get_service()
                        ->CreateReplaceUpdate(cube["name"], cube, false, true);

                if (res.size() != 1 || res[0].error_tag != kCreated) {
                    std::string msg;
                    for (auto &r: res) {
                        //logger->debug("- {}", r.message);
                        msg += std::string(r.message) + " ";
                    }

                    throw std::runtime_error("Error creating cube: " + cube["name"].get<std::string>() + msg);
                }
            }

            for (auto &[peer1, peer2] : peers) {
                core.try_to_set_peer(peer1, peer2);
            }
        }

        void RestServer::load_last_topology() {
            logger->info("loading cubes from {}",
                         configuration::config.getCubesDumpFile());
            std::ifstream topologyFile(configuration::config.getCubesDumpFile());
            if (!topologyFile.is_open()) {
                return;
            }

            std::stringstream buffer;
            buffer << topologyFile.rdbuf();
            topologyFile.close();
            // parse the file and create and initialize each cube one by one
            try {
                json jcubes = json::parse(buffer.str());
                logJson(jcubes);
                create_cubes(jcubes);
            } catch (const std::exception &e) {
                logger->error("{0}", e.what());
            }
        }

        void RestServer::setup_routes() {
            using Pistache::Rest::Routes::bind;
            router_->options(base + std::string("/"),
                             bind(&RestServer::root_handler, this));
            // servicectrls
            router_->post(base + std::string("/services"),
                          bind(&RestServer::post_servicectrl, this));
            router_->get(base + std::string("/services"),
                         bind(&RestServer::get_servicectrls, this));
            router_->get(base + std::string("/services/:name"),
                         bind(&RestServer::get_servicectrl, this));
            router_->del(base + std::string("/services/:name"),
                         bind(&RestServer::delete_servicectrl, this));

            // cubes
            router_->get(base + std::string("/cubes"),
                         bind(&RestServer::get_cubes, this));
            router_->get(base + std::string("/cubes/:cubeName"),
                         bind(&RestServer::get_cube, this));

            router_->post(base + std::string("/cubes"),
                          bind(&RestServer::post_cubes, this));

            router_->options(base + std::string("/cubes"),
                             bind(&RestServer::cubes_help, this));

            router_->options(base + std::string("/cubes/:cubeName"),
                             bind(&RestServer::cube_help, this));

            // netdevs
            router_->get(base + std::string("/netdevs"),
                         bind(&RestServer::get_netdevs, this));
            router_->get(base + std::string("/netdevs/:name"),
                         bind(&RestServer::get_netdev, this));

            router_->options(base + std::string("/netdevs"),
                             bind(&RestServer::netdevs_help, this));

            router_->options(base + std::string("/netdevs/:netdevName"),
                             bind(&RestServer::netdev_help, this));

            // version
            router_->get(base + std::string("/version"),
                         bind(&RestServer::get_version, this));

            // connect & disconnect
            router_->post(base + std::string("/connect"),
                          bind(&RestServer::connect, this));
            router_->post(base + std::string("/disconnect"),
                          bind(&RestServer::disconnect, this));

            router_->options(base + std::string("/connect"),
                             bind(&RestServer::connect_help, this));

            // attach & detach
            router_->post(base + std::string("/attach"), bind(&RestServer::attach, this));
            router_->post(base + std::string("/detach"), bind(&RestServer::detach, this));

            // topology
            router_->get(base + std::string("/topology"),
                         bind(&RestServer::topology, this));
            router_->get(base + std::string("/topology/:ifname"),
                         bind(&RestServer::get_if_topology, this));

            router_->options(base + std::string("/topology"),
                             bind(&RestServer::topology_help, this));

            router_->addNotFoundHandler(bind(&RestServer::redirect, this));

            //prometheus metrics
            router_->get(base + std::string("/metrics"),
                         bind(&RestServer::get_metrics, this));
        }

        void RestServer::logRequest(const Pistache::Rest::Request &request) {
// logger->debug("{0} : {1}", request.method(), request.resource());
#ifdef LOG_DEBUG_REQUEST_
            logger->debug(request.method() + ": " + request.resource());
            logger->debug(request.body());
#endif
        }

        void RestServer::logJson(json j) {
#ifdef LOG_DEBUG_JSON_
            logger->debug("JSON Dump: \n");
            logger->debug(j.dump());
#endif
        }

        void RestServer::root_handler(const Pistache::Rest::Request &request,
                                      Pistache::Http::ResponseWriter response) {
            auto help = request.query().get("help").getOrElse("NO_HELP");
            if (help == "NO_HELP") {
                Rest::Server::ResponseGenerator::Generate({{kOk, nullptr}},
                                                          std::move(response));
                return;
            }

            HelpType type;
            if (help == "NONE") {
                type = NONE;
            } else {
                Rest::Server::ResponseGenerator::Generate({{kBadRequest, nullptr}},
                                                          std::move(response));
                return;
            }

            auto completion = request.query().has("completion");
            if (!completion) {
                return root_help(type, std::move(response));
            } else {
                return root_completion(type, std::move(response));
            }
        }

        void RestServer::root_help(HelpType help_type,
                                   Pistache::Http::ResponseWriter response) {
            if (help_type == HelpType::NONE) {
                json j;
                auto services = core.get_servicectrls_list();
                for (auto &it : services) {
                    std::string service_name = it->get_name();
                    j["params"][service_name]["name"] = service_name + "provapino";
                    j["params"][service_name]["simpletype"] = "service";
                    j["params"][service_name]["type"] = "leaf";
                    j["params"][service_name]["description"] = it->get_description();
                    j["params"][service_name]["version"] = it->get_version();
                    j["params"][service_name]["pyang_repo_id"] = it->get_pyang_git_repo_id();
                    j["params"][service_name]["swagger_codegen_repo_id"] =
                            it->get_swagger_codegen_git_repo_id();
                }

                response.send(Pistache::Http::Code::Ok, j.dump());
            }
        }

        void RestServer::root_completion(HelpType help_type,
                                         Pistache::Http::ResponseWriter response) {
            if (help_type == HelpType::NONE) {
                json val = json::array();

                // fixed commands
                val += "connect";
                val += "disconnect";
                val += "attach";
                val += "detach";
                val += "services";
                val += "cubes";
                val += "topology";
                val += "netdevs";

                // services
                auto services = core.get_servicectrls_list();
                for (auto &it : services) {
                    val += it->get_name();
                }

                // cubes
                auto cubes = ServiceController::get_all_cubes();
                for (auto &it : cubes) {
                    val += it->get_name();
                }

                response.send(Pistache::Http::Code::Ok, val.dump());
            }
        }

        void RestServer::post_servicectrl(const Pistache::Rest::Request &request,
                                          Pistache::Http::ResponseWriter response) {
            logRequest(request);

            try {
                json j = json::parse(request.body());
                logJson(j);

                if (j.count("type") == 0) {
                    Rest::Server::ResponseGenerator::Generate(
                            std::vector<Response>{{ErrorTag::kMissingAttribute, strdup("type")}},
                            std::move(response));
                    return;
                }
                if (j.count("uri") == 0) {
                    Rest::Server::ResponseGenerator::Generate(
                            std::vector<Response>{{ErrorTag::kMissingAttribute, strdup("uri")}},
                            std::move(response));
                    return;
                }
                if (j.count("name") == 0) {
                    Rest::Server::ResponseGenerator::Generate(
                            std::vector<Response>{{ErrorTag::kMissingAttribute, strdup("name")}},
                            std::move(response));
                    return;
                }

                // parse type && URI (servicecontroller)
                auto jtype = j["type"].get<std::string>();
                auto uri = j["uri"].get<std::string>();
                auto name = j["name"].get<std::string>();

                ServiceControllerType type;
                if (jtype == "lib") {
                    type = ServiceControllerType::LIBRARY;
                } else if (jtype == "grpc") {
                    type = ServiceControllerType::DAEMON;
                } else {
                    Rest::Server::ResponseGenerator::Generate(
                            std::vector<Response>{{ErrorTag::kBadAttribute, strdup("type")}},
                            std::move(response));
                    return;
                }

                core.add_servicectrl(name, type, base, uri);
                response.send(Pistache::Http::Code::Ok);
            } catch (const std::runtime_error &e) {
                logger->error("{0}", e.what());
                response.send(Pistache::Http::Code::Bad_Request, e.what());
            }
        }

        void RestServer::get_servicectrls(const Pistache::Rest::Request &request,
                                          Pistache::Http::ResponseWriter response) {
            logRequest(request);
            try {
                std::string retJsonStr = core.get_servicectrls();
                response.send(Pistache::Http::Code::Ok, retJsonStr);
            } catch (const std::runtime_error &e) {
                logger->error("{0}", e.what());
                response.send(Pistache::Http::Code::Bad_Request, e.what());
            }
        }

        void RestServer::get_servicectrl(const Pistache::Rest::Request &request,
                                         Pistache::Http::ResponseWriter response) {
            logRequest(request);
            try {
                auto name = request.param(":name").as<std::string>();
                std::string retJsonStr = core.get_servicectrl(name);
                response.send(Pistache::Http::Code::Ok, retJsonStr);
            } catch (const std::runtime_error &e) {
                logger->error("{0}", e.what());
                response.send(Pistache::Http::Code::Bad_Request, e.what());
            }
        }

        void RestServer::delete_servicectrl(const Pistache::Rest::Request &request,
                                            Pistache::Http::ResponseWriter response) {
            logRequest(request);
            try {
                auto name = request.param(":name").as<std::string>();
                core.delete_servicectrl(name);
                response.send(Pistache::Http::Code::No_Content);
            } catch (const std::invalid_argument &e) {
                Rest::Server::ResponseGenerator::Generate(
                        std::vector<Response>{{ErrorTag::kDataMissing, nullptr}},
                        std::move(response));
            } catch (const std::runtime_error &e) {
                logger->error("{0}", e.what());
                response.send(Pistache::Http::Code::Bad_Request, e.what());
            }
        }

        void RestServer::get_cubes(const Pistache::Rest::Request &request,
                                   Pistache::Http::ResponseWriter response) {
            logRequest(request);
            try {
                std::string retJsonStr = core.get_cubes();
                response.send(Pistache::Http::Code::Ok, retJsonStr);
            } catch (const std::runtime_error &e) {
                logger->error("{0}", e.what());
                response.send(Pistache::Http::Code::Bad_Request, e.what());
            }
        }

        void RestServer::get_cube(const Pistache::Rest::Request &request,
                                  Pistache::Http::ResponseWriter response) {
            logRequest(request);
            try {
                auto name = request.param(":cubeName").as<std::string>();
                std::string retJsonStr = core.get_cube(name);
                response.send(Pistache::Http::Code::Ok, retJsonStr);
            } catch (const std::runtime_error &e) {
                logger->error("{0}", e.what());
                response.send(Pistache::Http::Code::Bad_Request, e.what());
            }
        }

        void RestServer::post_cubes(const Pistache::Rest::Request &request,
                                    Pistache::Http::ResponseWriter response) {
            logRequest(request);
            try {
                json cubes = json::parse(request.body());
                logJson(cubes);
                create_cubes(cubes);
            } catch (const std::runtime_error &e) {
                logger->error("{0}", e.what());
                response.send(Pistache::Http::Code::Bad_Request, e.what());
            }
        }
        void RestServer::create_metrics() {
            logger->info("loading metrics from yang files");
            try {
                //uri and metricHandler are not necessary, I use pistache
                //to registry metrics
                registry = std::make_shared<prometheus::Registry>();
                std::vector<std::string> name_services = core.get_servicectrls_names_vector();

                for (int i = 0; i < name_services.size(); i++) {
                    auto metrics_service = core.get_service_controller(name_services[i]).get_infoMetrics();

                    for (int j = 0; j < metrics_service.size(); j++) {

                        if (metrics_service[j].typeMetric == "C") {
                            map_metrics[name_services[i]].counters_family_.push_back(prometheus::BuildCounter()
                                                                                             .Name(metrics_service[j].nameMetric)
                                                                                             .Help(metrics_service[j].helpMetric)
                                                                                             .Labels({})
                                                                                             .Register(*registry));

                            //I can access directly to a counter of a family without add a counter
                            //counters_family_[i].get().Add({}).Increment();

                            //I add a counter to te family
                            //In this way I can have a family but many counter with different label=value
                            map_metrics[name_services[i]].counters_.push_back(
                                    map_metrics[name_services[i]].counters_family_.back().get().Add({}));

                            //map_metrics[name_services[i]].counters_.back().get().Increment(j);
                        } else if (metrics_service[j].typeMetric == "G") {

                            map_metrics[name_services[i]].gauges_family_.push_back(prometheus::BuildGauge()
                                                                                           .Name(metrics_service[j].nameMetric)
                                                                                           .Help(metrics_service[j].helpMetric)
                                                                                           .Labels({})
                                                                                           .Register(*registry));
                            //I can access directly to a counter of a family without add a counter
                            //counters_family_[i].get().Add({}).Increment();

                            //I add a counter to te family
                            //In this way I can have a family but many counter with different label=value
                            map_metrics[name_services[i]].gauges_.push_back(
                                    map_metrics[name_services[i]].gauges_family_.back().get().Add({}));

                            //map_metrics[name_services[i]].gauges_.back().get().Set(1);
                        }
                    }
                }
                //all metrics created are put into collectalbes_ thanks to registry
                //collectables_ can collects al types of metrics (counter, gauge, histogram, summary)
                collectables_.push_back(registry);
            }
            catch (const std::runtime_error &e) {
                logger->error("{0}", e.what());
            }
        }

//expose OpenMetrics
        void RestServer::get_metrics(const Pistache::Rest::Request &request,
                                     Pistache::Http::ResponseWriter response) {
            logRequest(request);
            try {
                //TODO
                //Parse the string of data into a json value
                //std::string temp = core.get_cube("d1");
                //jsoncons::json j = jsoncons::json::parse(core.get_cube("d1"));
                //jsoncons::json result = jsoncons::jsonpath::json_query(j,"$.uuid");
                //std::cout << "result " << result << "\n";

                //vector of MetricFamily
                auto collected_metrics = std::vector<prometheus::MetricFamily>{};
                jsoncons::json jk;
                //collectables_ is filled in crete_metrics
                for (auto &&wcollectable : collectables_) {
                    auto collectable = wcollectable.lock();
                    if (!collectable) {
                        continue;
                    }
                    //Returns a list of metrics and their samples.  std::vector<prometheus::MetricFamily> &&metrics
                    auto &&metrics = collectable->Collect();
                    collected_metrics.insert(collected_metrics.end(),
                                             std::make_move_iterator(metrics.begin()),
                                             std::make_move_iterator(metrics.end()));
                }
                //auto metrics = collected_metrics;
                auto serializer = std::unique_ptr<prometheus::Serializer>{new prometheus::TextSerializer()};
                std::string ret_metrics = serializer->Serialize(collected_metrics);

                response.send(Pistache::Http::Code::Ok, ret_metrics);
            } catch (const std::runtime_error &e) {
                logger->error("{0}", e.what());
                response.send(Pistache::Http::Code::Bad_Request, e.what());
            }
        }


        void RestServer::cubes_help(const Pistache::Rest::Request &request,
                                    Pistache::Http::ResponseWriter response) {
            json j = json::object();
            auto help = request.query().get("help").getOrElse("NO_HELP");
            if (help != "NONE" && help != "SHOW") {
                response.send(Pistache::Http::Code::Bad_Request);
                return;
            }

            json params = json::object();
            params["name"]["name"] = "name";
            params["name"]["type"] = "key";
            params["name"]["simpletype"] = "string";
            params["name"]["description"] = "Cube Name";
            params["name"]["example"] = "cube0";

            j["params"] = params;

            json j_cubes = json::array();
            auto cubes = ServiceController::get_all_cubes();
            for (auto &it : cubes) {
                json j_cube = json::object();
                j_cube["cube"] = it->get_name();
                j_cubes += j_cube;
            }

            j["elements"] = j_cubes;

            if (help == "NONE") {
                j["commands"] = {"show"};
            }

            response.send(Pistache::Http::Code::Ok, j.dump());
        }

        void RestServer::cube_help(const Pistache::Rest::Request &request,
                                   Pistache::Http::ResponseWriter response) {
            json j = json::object();
            auto help = request.query().get("help").getOrElse("NO_HELP");
            if (help != "NONE") {
                response.send(Pistache::Http::Code::Bad_Request);
                return;
            }

            j["commands"] = {"show"};

            response.send(Pistache::Http::Code::Ok, j.dump());
        }

        void RestServer::get_netdevs(const Pistache::Rest::Request &request,
                                     Pistache::Http::ResponseWriter response) {
            logRequest(request);
            try {
                std::string retJsonStr = core.get_netdevs();
                response.send(Pistache::Http::Code::Ok, retJsonStr);
            } catch (const std::runtime_error &e) {
                logger->error("{0}", e.what());
                response.send(Pistache::Http::Code::Bad_Request, e.what());
            }
        }

        void RestServer::get_netdev(const Pistache::Rest::Request &request,
                                    Pistache::Http::ResponseWriter response) {
            logRequest(request);
            try {
                auto name = request.param(":name").as<std::string>();
                std::string retJsonStr = core.get_netdev(name);
                response.send(Pistache::Http::Code::Ok, retJsonStr);
            } catch (const std::runtime_error &e) {
                logger->error("{0}", e.what());
                response.send(Pistache::Http::Code::Bad_Request, e.what());
            }
        }

        void RestServer::netdevs_help(const Pistache::Rest::Request &request,
                                      Pistache::Http::ResponseWriter response) {
            json j = json::object();
            auto help = request.query().get("help").getOrElse("NO_HELP");
            if (help != "NONE" && help != "SHOW") {
                response.send(Pistache::Http::Code::Bad_Request);
                return;
            }

            json params = json::object();
            params["name"]["name"] = "name";
            params["name"]["type"] = "key";
            params["name"]["simpletype"] = "string";
            params["name"]["description"] = "Netdev name";
            params["name"]["example"] = "eth0";
            j["params"] = params;

            auto netdevs = Netlink::getInstance().get_available_ifaces();

            json j_netdevs = json::array();
            for (auto &it : netdevs) {
                json j_netdev = json::object();
                j_netdev["netdev"] = it.first;
                j_netdevs += j_netdev;
            }

            j["elements"] = j_netdevs;

            if (help == "NONE") {
                j["commands"] = {"show"};
            }

            response.send(Pistache::Http::Code::Ok, j.dump());
        }

        void RestServer::netdev_help(const Pistache::Rest::Request &request,
                                     Pistache::Http::ResponseWriter response) {
            json j = json::object();
            auto help = request.query().get("help").getOrElse("NO_HELP");
            if (help != "NONE") {
                response.send(Pistache::Http::Code::Bad_Request);
                return;
            }

            j["commands"] = {"show"};

            response.send(Pistache::Http::Code::Ok, j.dump());
        }

        void RestServer::get_version(const Pistache::Rest::Request &request,
                                     Pistache::Http::ResponseWriter response) {
            json version = json::object();
            version["polycubed"]["version"] = VERSION;
            response.send(Pistache::Http::Code::Ok, version.dump());
        }

        void RestServer::connect(const Pistache::Rest::Request &request,
                                 Pistache::Http::ResponseWriter response) {
            logRequest(request);
            std::string peer1, peer2;

            nlohmann::json val = nlohmann::json::parse(request.body());

            if (val.find("peer1") == val.end()) {
                response.send(Pistache::Http::Code::Bad_Request, "Peer 1 is missing");
            }

            peer1 = val.at("peer1");

            if (val.find("peer2") == val.end()) {
                response.send(Pistache::Http::Code::Bad_Request, "Peer 2 is missing");
            }

            peer2 = val.at("peer2");

            try {
                core.connect(peer1, peer2);
                response.send(Pistache::Http::Code::Ok);
            } catch (const std::runtime_error &e) {
                logger->error("{0}", e.what());
                response.send(Pistache::Http::Code::Bad_Request, e.what());
            }
        }

        void RestServer::disconnect(const Pistache::Rest::Request &request,
                                    Pistache::Http::ResponseWriter response) {
            logRequest(request);

            std::string peer1, peer2;

            nlohmann::json val = nlohmann::json::parse(request.body());

            if (val.find("peer1") == val.end()) {
                response.send(Pistache::Http::Code::Bad_Request, "Peer 1 is missing");
            }

            peer1 = val.at("peer1");

            if (val.find("peer2") == val.end()) {
                response.send(Pistache::Http::Code::Bad_Request, "Peer 2 is missing");
            }

            peer2 = val.at("peer2");

            try {
                core.disconnect(peer1, peer2);
                response.send(Pistache::Http::Code::Ok);
            } catch (const std::runtime_error &e) {
                logger->error("{0}", e.what());
                response.send(Pistache::Http::Code::Bad_Request, e.what());
            }
        }

        void RestServer::connect_help(const Pistache::Rest::Request &request,
                                      Pistache::Http::ResponseWriter response) {
            auto help = request.query().get("help").getOrElse("NO_HELP");
            if (help != "NONE") {
                response.send(Pistache::Http::Code::Bad_Request);
                return;
            }

            auto completion = request.query().has("completion");
            if (completion) {
                return connect_completion("", std::move(response));
            }

            response.send(Pistache::Http::Code::Ok, "not implemented");;
        }

        void RestServer::connect_completion(const std::string &p1,
                                            Pistache::Http::ResponseWriter response) {
            auto ports = core.get_all_ports();

            if (!p1.empty()) {
                std::string p(p1);
                //ports.erase(p);
                ports.erase(std::remove(ports.begin(), ports.end(), p1), ports.end());
            }

            json val = ports;
            response.send(Pistache::Http::Code::Ok, val.dump());
        }

        void RestServer::attach(const Pistache::Rest::Request &request,
                                Pistache::Http::ResponseWriter response) {
            logRequest(request);

            std::string cube, port, position, other;
            bool position_(false);

            nlohmann::json val = nlohmann::json::parse(request.body());

            if (val.find("cube") == val.end()) {
                response.send(Pistache::Http::Code::Bad_Request, "Cube is missing");
                return;
            }

            cube = val.at("cube");

            if (val.find("port") == val.end()) {
                response.send(Pistache::Http::Code::Bad_Request, "Port is missing");
                return;
            }

            port = val.at("port");

            position = "auto";  // default value

            if (val.find("position") != val.end()) {
                position = val.at("position");
                position_ = true;
            }

            if (val.find("after") != val.end()) {
                if (position_) {
                    response.send(Pistache::Http::Code::Bad_Request,
                                  "position and after cannot be used together");
                    return;
                }

                position = "after";
                other = val.at("after");
            }

            if (val.find("before") != val.end()) {
                if (position_) {
                    response.send(Pistache::Http::Code::Bad_Request,
                                  "position and before cannot be used together");
                    return;
                }

                if (!other.empty()) {
                    response.send(Pistache::Http::Code::Bad_Request,
                                  "after and before cannot be used together");
                    return;
                }

                position = "before";
                other = val.at("before");
            }

            try {
                core.attach(cube, port, position, other);
                response.send(Pistache::Http::Code::Ok);
            } catch (const std::runtime_error &e) {
                logger->error("{0}", e.what());
                response.send(Pistache::Http::Code::Bad_Request, e.what());
            }
        }

        void RestServer::detach(const Pistache::Rest::Request &request,
                                Pistache::Http::ResponseWriter response) {
            logRequest(request);

            std::string cube, port;

            nlohmann::json val = nlohmann::json::parse(request.body());

            if (val.find("cube") == val.end()) {
                response.send(Pistache::Http::Code::Bad_Request, "Cube is missing");
                return;
            }

            cube = val.at("cube");

            if (val.find("port") == val.end()) {
                response.send(Pistache::Http::Code::Bad_Request, "Port is missing");
                return;
            }

            port = val.at("port");

            try {
                core.detach(cube, port);
                response.send(Pistache::Http::Code::Ok);
            } catch (const std::runtime_error &e) {
                logger->error("{0}", e.what());
                response.send(Pistache::Http::Code::Bad_Request, e.what());
            }
        }

        void RestServer::topology(const Pistache::Rest::Request &request,
                                  Pistache::Http::ResponseWriter response) {
            logRequest(request);
            try {
                std::string retJsonStr = core.topology();
                response.send(Pistache::Http::Code::Ok, retJsonStr);
            } catch (const std::runtime_error &e) {
                logger->error("{0}", e.what());
                response.send(Pistache::Http::Code::Bad_Request, e.what());
            }
        }

        void RestServer::get_if_topology(const Pistache::Rest::Request &request,
                                         Pistache::Http::ResponseWriter response) {
            logRequest(request);
            try {
                auto ifname = request.param(":ifname").as<std::string>();
                std::string retJsonStr = core.get_if_topology(ifname);
                response.send(Pistache::Http::Code::Ok, retJsonStr);
            } catch (const std::runtime_error &e) {
                logger->error("{0}", e.what());
                response.send(Pistache::Http::Code::Bad_Request, e.what());
            }
        }

        void RestServer::topology_help(const Pistache::Rest::Request &request,
                                       Pistache::Http::ResponseWriter response) {
            json j = json::object();
            auto help = request.query().get("help").getOrElse("NO_HELP");
            if (help != "NONE") {
                response.send(Pistache::Http::Code::Bad_Request);
                return;
            }

            j["commands"] = {"show"};

            response.send(Pistache::Http::Code::Ok, j.dump());
        }

        void RestServer::redirect(const Pistache::Rest::Request &request,
                                  Pistache::Http::ResponseWriter response) {
            if (request.headers().has("Referer") ||
                request.headers().rawList().count("Referer") == 1) {
                response.send(Pistache::Http::Code::Not_Found,
                              "Cannot find a matching route.");
                return;
            }
            std::string url = request.resource();
            if (url.find(base) == std::string::npos) {
                response.send(Pistache::Http::Code::Not_Found,
                              "Cannot find a matching route.");
                return;
            }
            auto base_length = base.length();
            std::string cube_name =
                    url.substr(base_length, url.find('/', base_length) - base_length);
            const auto &service_name = ServiceController::get_cube_service(cube_name);
            if (service_name.length() == 0) {
                response.send(Pistache::Http::Code::Not_Found,
                              "No cube found named " + cube_name);
                return;
            }
            std::string redirect = request.resource() + request.query().as_str();

            redirect.insert(base_length, service_name + '/');
            auto location = Pistache::Http::Header::Location(redirect);
            response.headers().add<Pistache::Http::Header::Location>(location);
            response.send(Pistache::Http::Code::Permanent_Redirect);
        }

    }  // namespace polycubed
}  // namespace polycube
