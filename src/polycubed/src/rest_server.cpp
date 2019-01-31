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
#include "service_controller.h"
#include "version.h"

namespace polycube {
namespace polycubed {

using polycube::service::HelpType;

// these variables are neded in the client_verify callback, they need to be
// static.
std::string RestServer::whitelist_cert_path;
std::string RestServer::blacklist_cert_path;

// start http server for Management APIs
// Incapsultate a core object // TODO probably there are best ways...
RestServer::RestServer(Address addr, PolycubedCore &core)
    : core(core),
      httpEndpoint(std::make_shared<Http::Endpoint>(addr)),
      logger(spdlog::get("polycubed")) {
  logger->info("rest server listening on '{0}:{1}'", addr.host(), addr.port());
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
  X509_STORE_CTX *x509_ctx = (X509_STORE_CTX *)ctx;
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
  auto opts = Http::Endpoint::options().threads(thr).flags(
      Tcp::Options::InstallSignalHandler | Tcp::Options::ReuseAddr);
  httpEndpoint->init(opts);

  if (!server_cert.empty()) {
    httpEndpoint->useSSL(server_cert, server_key);
  }

  if (!root_ca_cert.empty() || !whitelist_cert_path_.empty() ||
      !blacklist_cert_path_.empty()) {
    httpEndpoint->useSSLAuth(root_ca_cert, "", client_verify_callback);
  }

  RestServer::whitelist_cert_path = whitelist_cert_path_;
  RestServer::blacklist_cert_path = blacklist_cert_path_;

  setup_routes();
}

void RestServer::start() {
  logger->info("rest server starting ...");
  httpEndpoint->setHandler(router.handler());
  // httpEndpoint->serve();
  httpEndpoint->serveThreaded();
}

void RestServer::shutdown() {
  logger->info("shutting down rest server ...");
  try {
    httpEndpoint->shutdown();
  } catch (const std::runtime_error &e) {
    logger->error("{0}", e.what());
  }
}

void RestServer::setup_routes() {
  using namespace Pistache::Rest;

  Routes::Options(router, base + std::string("/"),
                  Routes::bind(&RestServer::root_handler, this));
  // servicectrls
  Routes::Post(router, base + std::string("/services"),
               Routes::bind(&RestServer::post_servicectrl, this));
  Routes::Get(router, base + std::string("/services"),
              Routes::bind(&RestServer::get_servicectrls, this));
  Routes::Get(router, base + std::string("/services/:name"),
              Routes::bind(&RestServer::get_servicectrl, this));
  Routes::Delete(router, base + std::string("/services/:name"),
                 Routes::bind(&RestServer::delete_servicectrl, this));

  // cubes
  Routes::Get(router, base + std::string("/cubes"),
              Routes::bind(&RestServer::get_cubes, this));
  Routes::Get(router, base + std::string("/cubes/:cubeName"),
              Routes::bind(&RestServer::get_cube, this));

  Routes::Options(router, base + std::string("/cubes"),
                  Routes::bind(&RestServer::cubes_help, this));

  Routes::Options(router, base + std::string("/cubes/:cubeName"),
                  Routes::bind(&RestServer::cube_help, this));

  // netdevs
  Routes::Get(router, base + std::string("/netdevs"),
              Routes::bind(&RestServer::get_netdevs, this));
  Routes::Get(router, base + std::string("/netdevs/:name"),
              Routes::bind(&RestServer::get_netdev, this));

  Routes::Options(router, base + std::string("/netdevs"),
                  Routes::bind(&RestServer::netdevs_help, this));

  Routes::Options(router, base + std::string("/netdevs/:netdevName"),
                  Routes::bind(&RestServer::netdev_help, this));

  // version
  Routes::Get(router, base + std::string("/version"),
              Routes::bind(&RestServer::get_version, this));

  // connect & disconnect
  Routes::Post(router, base + std::string("/connect"),
               Routes::bind(&RestServer::connect, this));
  Routes::Post(router, base + std::string("/disconnect"),
               Routes::bind(&RestServer::disconnect, this));

  // attach & detach
  Routes::Post(router, base + std::string("/attach"),
               Routes::bind(&RestServer::attach, this));
  Routes::Post(router, base + std::string("/detach"),
               Routes::bind(&RestServer::detach, this));

  // topology
  Routes::Get(router, base + std::string("/topology"),
              Routes::bind(&RestServer::topology, this));

  Routes::Options(router, base + std::string("/topology"),
                  Routes::bind(&RestServer::topology_help, this));
}

void RestServer::logRequest(const Rest::Request &request) {
// logger->debug("{0} : {1}", request.method(), request.resource());
#ifdef LOG_DEBUG_REQUEST_
  logger->debug(request.method() + ": " + request.resource());
  logger->debug(request.body());
#endif
}

void RestServer::logJson(json j) {
#ifdef LOG_DEBUG_JSON_
  logger->debug("JSON Dump: ");
  << std::endl;
  logger->debug(j.dump(4));
#endif
}

void RestServer::root_handler(const Rest::Request &request,
                              Http::ResponseWriter response) {
  HelpType help_type;
  auto help = request.query().get("help").getOrElse("NO_HELP");
  if (help.compare("NO_HELP") == 0)
    help_type = HelpType::NO_HELP;
  else if (help.compare("NONE") == 0)
    help_type = HelpType::NONE;
  else {
    response.send(Http::Code::Bad_Request);
    return;
  }

  if (help_type == HelpType::NONE) {
    json j;
    auto services = core.get_servicectrls_list();
    for (auto &it : services) {
      std::string service_name = it->get_name();
      j["params"][service_name]["name"] = service_name;
      j["params"][service_name]["simpletype"] = "service";
      j["params"][service_name]["type"] = "leaf";
      j["params"][service_name]["description"] = it->get_description();
      j["params"][service_name]["version"] = it->get_version();
      j["params"][service_name]["pyang_repo_id"] = it->get_pyang_git_repo_id();
      j["params"][service_name]["swagger_codegen_repo_id"] =
          it->get_swagger_codegen_git_repo_id();
    }

    response.send(Http::Code::Ok, j.dump(4));
  }
}

void RestServer::post_servicectrl(const Rest::Request &request,
                                  Http::ResponseWriter response) {
  logRequest(request);

  try {
    json j = json::parse(request.body());
    logJson(j);
    auto name = j["name"].get<std::string>();
    auto servicecontroller = j["servicecontroller"].get<std::string>();

    core.add_servicectrl(name, servicecontroller);
    response.send(Http::Code::Ok);
  } catch (const std::runtime_error &e) {
    logger->error("{0}", e.what());
    response.send(Http::Code::Bad_Request, e.what());
  }
}

void RestServer::get_servicectrls(const Rest::Request &request,
                                  Http::ResponseWriter response) {
  logRequest(request);
  try {
    std::string retJsonStr = core.get_servicectrls();
    response.send(Http::Code::Ok, retJsonStr);
  } catch (const std::runtime_error &e) {
    logger->error("{0}", e.what());
    response.send(Http::Code::Bad_Request, e.what());
  }
}

void RestServer::get_servicectrl(const Rest::Request &request,
                                 Http::ResponseWriter response) {
  logRequest(request);
  try {
    auto name = request.param(":name").as<std::string>();
    std::string retJsonStr = core.get_servicectrl(name);
    response.send(Http::Code::Ok, retJsonStr);
  } catch (const std::runtime_error &e) {
    logger->error("{0}", e.what());
    response.send(Http::Code::Bad_Request, e.what());
  }
}

void RestServer::delete_servicectrl(const Rest::Request &request,
                                    Http::ResponseWriter response) {
  logRequest(request);
  try {
    auto name = request.param(":name").as<std::string>();
    core.delete_servicectrl(name);
    response.send(Http::Code::Ok);
  } catch (const std::runtime_error &e) {
    logger->error("{0}", e.what());
    response.send(Http::Code::Bad_Request, e.what());
  }
}

void RestServer::get_cubes(const Rest::Request &request,
                           Http::ResponseWriter response) {
  logRequest(request);
  try {
    std::string retJsonStr = core.get_cubes();
    response.send(Http::Code::Ok, retJsonStr);
  } catch (const std::runtime_error &e) {
    logger->error("{0}", e.what());
    response.send(Http::Code::Bad_Request, e.what());
  }
}

void RestServer::get_cube(const Rest::Request &request,
                          Http::ResponseWriter response) {
  logRequest(request);
  try {
    auto name = request.param(":cubeName").as<std::string>();
    std::string retJsonStr = core.get_cube(name);
    response.send(Http::Code::Ok, retJsonStr);
  } catch (const std::runtime_error &e) {
    logger->error("{0}", e.what());
    response.send(Http::Code::Bad_Request, e.what());
  }
}

void RestServer::cubes_help(const Rest::Request &request,
                            Http::ResponseWriter response) {
  json j = json::object();
  auto help = request.query().get("help").getOrElse("NO_HELP");
  if (help != "NONE" && help != "SHOW") {
    response.send(Http::Code::Bad_Request);
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

  response.send(Http::Code::Ok, j.dump(4));
}

void RestServer::cube_help(const Rest::Request &request,
                           Http::ResponseWriter response) {
  json j = json::object();
  auto help = request.query().get("help").getOrElse("NO_HELP");
  if (help != "NONE") {
    response.send(Http::Code::Bad_Request);
    return;
  }

  j["commands"] = {"show"};

  response.send(Http::Code::Ok, j.dump(4));
}

void RestServer::get_netdevs(const Rest::Request &request,
                             Http::ResponseWriter response) {
  logRequest(request);
  try {
    std::string retJsonStr = core.get_netdevs();
    response.send(Http::Code::Ok, retJsonStr);
  } catch (const std::runtime_error &e) {
    logger->error("{0}", e.what());
    response.send(Http::Code::Bad_Request, e.what());
  }
}

void RestServer::get_netdev(const Rest::Request &request,
                            Http::ResponseWriter response) {
  logRequest(request);
  try {
    auto name = request.param(":name").as<std::string>();
    std::string retJsonStr = core.get_netdev(name);
    response.send(Http::Code::Ok, retJsonStr);
  } catch (const std::runtime_error &e) {
    logger->error("{0}", e.what());
    response.send(Http::Code::Bad_Request, e.what());
  }
}

void RestServer::netdevs_help(const Rest::Request &request,
                              Http::ResponseWriter response) {
  json j = json::object();
  auto help = request.query().get("help").getOrElse("NO_HELP");
  if (help != "NONE" && help != "SHOW") {
    response.send(Http::Code::Bad_Request);
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

  response.send(Http::Code::Ok, j.dump(4));
}

void RestServer::netdev_help(const Rest::Request &request,
                             Http::ResponseWriter response) {
  json j = json::object();
  auto help = request.query().get("help").getOrElse("NO_HELP");
  if (help != "NONE") {
    response.send(Http::Code::Bad_Request);
    return;
  }

  j["commands"] = {"show"};

  response.send(Http::Code::Ok, j.dump(4));
}

void RestServer::get_version(const Rest::Request &request,
                             Http::ResponseWriter response) {
  json version = json::object();
  version["polycubed"]["version"] = VERSION;
  response.send(Http::Code::Ok, version.dump(4));
}

void RestServer::connect(const Rest::Request &request,
                         Http::ResponseWriter response) {
  logRequest(request);
  std::string peer1, peer2;

  nlohmann::json val = nlohmann::json::parse(request.body());

  if (val.find("peer1") == val.end()) {
    response.send(Http::Code::Bad_Request, "Peer 1 is missing");
  }

  peer1 = val.at("peer1");

  if (val.find("peer2") == val.end()) {
    response.send(Http::Code::Bad_Request, "Peer 2 is missing");
  }

  peer2 = val.at("peer2");

  try {
    core.connect(peer1, peer2);
    response.send(Http::Code::Ok);
  } catch (const std::runtime_error &e) {
    logger->error("{0}", e.what());
    response.send(Http::Code::Bad_Request, e.what());
  }
}

void RestServer::disconnect(const Rest::Request &request,
                            Http::ResponseWriter response) {
  logRequest(request);

  std::string peer1, peer2;

  nlohmann::json val = nlohmann::json::parse(request.body());

  if (val.find("peer1") == val.end()) {
    response.send(Http::Code::Bad_Request, "Peer 1 is missing");
  }

  peer1 = val.at("peer1");

  if (val.find("peer2") == val.end()) {
    response.send(Http::Code::Bad_Request, "Peer 2 is missing");
  }

  peer2 = val.at("peer2");

  try {
    core.disconnect(peer1, peer2);
    response.send(Http::Code::Ok);
  } catch (const std::runtime_error &e) {
    logger->error("{0}", e.what());
    response.send(Http::Code::Bad_Request, e.what());
  }
}

void RestServer::attach(const Rest::Request &request,
                        Http::ResponseWriter response) {
  logRequest(request);

  std::string cube, port, position, other;
  bool position_(false);

  nlohmann::json val = nlohmann::json::parse(request.body());

  if (val.find("cube") == val.end()) {
    response.send(Http::Code::Bad_Request, "Cube is missing");
    return;
  }

  cube = val.at("cube");

  if (val.find("port") == val.end()) {
    response.send(Http::Code::Bad_Request, "Port is missing");
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
      response.send(Http::Code::Bad_Request,
                    "position and after cannot be used together");
      return;
    }

    position = "after";
    other = val.at("after");
  }

  if (val.find("before") != val.end()) {
    if (position_) {
      response.send(Http::Code::Bad_Request,
                    "position and before cannot be used together");
      return;
    }

    if (!other.empty()) {
      response.send(Http::Code::Bad_Request,
                    "after and before cannot be used together");
      return;
    }

    position = "before";
    other = val.at("before");
  }

  try {
    core.attach(cube, port, position, other);
    response.send(Http::Code::Ok);
  } catch (const std::runtime_error &e) {
    logger->error("{0}", e.what());
    response.send(Http::Code::Bad_Request, e.what());
  }
}

void RestServer::detach(const Rest::Request &request,
                        Http::ResponseWriter response) {
  logRequest(request);

  std::string cube, port;

  nlohmann::json val = nlohmann::json::parse(request.body());

  if (val.find("cube") == val.end()) {
    response.send(Http::Code::Bad_Request, "Cube is missing");
    return;
  }

  cube = val.at("cube");

  if (val.find("port") == val.end()) {
    response.send(Http::Code::Bad_Request, "Port is missing");
    return;
  }

  port = val.at("port");

  try {
    core.detach(cube, port);
    response.send(Http::Code::Ok);
  } catch (const std::runtime_error &e) {
    logger->error("{0}", e.what());
    response.send(Http::Code::Bad_Request, e.what());
  }
}

void RestServer::topology(const Rest::Request &request,
                          Http::ResponseWriter response) {
  logRequest(request);
  try {
    std::string retJsonStr = core.topology();
    response.send(Http::Code::Ok, retJsonStr);
  } catch (const std::runtime_error &e) {
    logger->error("{0}", e.what());
    response.send(Http::Code::Bad_Request, e.what());
  }
}

void RestServer::topology_help(const Rest::Request &request,
                               Http::ResponseWriter response) {
  json j = json::object();
  auto help = request.query().get("help").getOrElse("NO_HELP");
  if (help != "NONE") {
    response.send(Http::Code::Bad_Request);
    return;
  }

  j["commands"] = {"show"};

  response.send(Http::Code::Ok, j.dump(4));
}

}  // namespace polycubed
}  // namespace polycube
