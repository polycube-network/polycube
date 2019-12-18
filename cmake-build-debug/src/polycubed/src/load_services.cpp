#include "polycubed_core.h"

#include <spdlog/spdlog.h>

using namespace polycube::polycubed;

void load_services(PolycubedCore &core) {
  std::shared_ptr<spdlog::logger> logger = spdlog::get("polycubed");

  // tries to load a service, in case of error just print and
  // continue
  #define try_to_load(_name, _path)                                   \
    do {                                                              \
      try {                                                           \
        core.add_servicectrl(_name, ServiceControllerType::LIBRARY,   \
                            "/polycube/v1/", _path);                  \
      } catch (const std::exception &e) {                             \
        logger->error("Error loading {0}: {1}", _name, e.what());     \
      };                                                              \
    } while (0)

  try_to_load("bridge", "libpcn-bridge.so");
try_to_load("ddosmitigator", "libpcn-ddosmitigator.so");
try_to_load("firewall", "libpcn-firewall.so");
try_to_load("helloworld", "libpcn-helloworld.so");
try_to_load("k8switch", "libpcn-k8switch.so");
try_to_load("k8sfilter", "libpcn-k8sfilter.so");
try_to_load("lbdsr", "libpcn-lbdsr.so");
try_to_load("lbrp", "libpcn-lbrp.so");
try_to_load("nat", "libpcn-nat.so");
try_to_load("pbforwarder", "libpcn-pbforwarder.so");
try_to_load("router", "libpcn-router.so");
try_to_load("simplebridge", "libpcn-simplebridge.so");
try_to_load("simpleforwarder", "libpcn-simpleforwarder.so");
try_to_load("iptables", "libpcn-iptables.so");
try_to_load("transparenthelloworld", "libpcn-transparenthelloworld.so");
try_to_load("synflood", "libpcn-synflood.so");
try_to_load("packetcapture", "libpcn-packetcapture.so");


  #undef try_to_load
}
