#include "cube.h"

#include "port_tc.h"
#include "port_xdp.h"

namespace polycube {
namespace polycubed {

#ifdef LOG_COMPILEED_CODE
void Cube::log_compileed_code(std::string &code) {
  int i = 0;
  int lines_not_empty = 0;

  std::string well_formatted_code;
  std::istringstream iss(code);
  for (std::string line; std::getline(iss, line); ++i) {
    well_formatted_code += std::to_string(i) + ": " + line + "\n";
    if (line != "")
      lines_not_empty++;
  }

  logger->info("C code compileed\n");
  logger->info(
      "\n--------------------------------------\n{0}\n-------------------------"
      "--------\n",
      well_formatted_code);

  logger->info("+Lines of C code: {0}", i);
  logger->info("+Lines of C code (not empty): {0}", lines_not_empty);
}
#endif

std::vector<std::string> Cube::cflags = {
    std::string("-D_POLYCUBE_MAX_NODES=") +
        std::to_string(Node::_POLYCUBE_MAX_NODES),
    std::string("-D_POLYCUBE_MAX_PORTS=") + std::to_string(_POLYCUBE_MAX_PORTS),
    std::string("-D_POLYCUBE_MAX_BPF_PROGRAMS=") +
        std::to_string(_POLYCUBE_MAX_BPF_PROGRAMS),
};

Cube::Cube(const std::string &name, const std::string &service_name,
           PatchPanel &patch_panel_ingress, PatchPanel &patch_panel_egress,
           LogLevel level, CubeType type)
    : name_(name),
      service_name_(service_name),
      logger(spdlog::get("polycubed")),
      uuid_(GuidGenerator().newGuid()),
      patch_panel_ingress_(patch_panel_ingress),
      patch_panel_egress_(patch_panel_egress),
      ingress_fd_(0),
      egress_fd_(0),
      ingress_index_(0),
      egress_index_(0),
      level_(level),
      type_(type),
      id_(id_generator_.acquire()) {
  std::lock_guard<std::mutex> guard(bcc_mutex);

  // create master program that contains some maps definitions
  master_program_ =
      std::unique_ptr<ebpf::BPF>(new ebpf::BPF(0, nullptr, false, name));
  master_program_->init(MASTER_CODE, cflags);

  // get references to those maps
  auto ingress_ = master_program_->get_prog_table("ingress_programs");
  ingress_programs_table_ =
      std::unique_ptr<ebpf::BPFProgTable>(new ebpf::BPFProgTable(ingress_));

  auto egress_ = master_program_->get_prog_table("egress_programs");
  egress_programs_table_ =
      std::unique_ptr<ebpf::BPFProgTable>(new ebpf::BPFProgTable(egress_));

  auto forward_ = master_program_->get_array_table<uint32_t>("forward_chain_");
  forward_chain_ = std::unique_ptr<ebpf::BPFArrayTable<uint32_t>>(
      new ebpf::BPFArrayTable<uint32_t>(forward_));

  // add free ports
  for (uint16_t i = 0; i < _POLYCUBE_MAX_PORTS; i++)
    free_ports_.insert(i);
}

void Cube::init(const std::vector<std::string> &ingress_code,
                const std::vector<std::string> &egress_code) {
  for (int i = 0; i < ingress_code.size(); i++) {
    add_program(ingress_code[i], i, ProgramType::INGRESS);
  }

  for (int i = 0; i < egress_code.size(); i++) {
    add_program(egress_code[i], i, ProgramType::EGRESS);
  }
}

void Cube::uninit() {
  if (ingress_index_)
    patch_panel_ingress_.remove(ingress_index_);

  if (egress_index_)
    patch_panel_egress_.remove(egress_index_);

  for (int i = 0; i < ingress_programs_.size(); i++) {
    if (ingress_programs_[i]) {
      del_program(i, ProgramType::INGRESS);
    }
  }

  for (int i = 0; i < egress_programs_.size(); i++) {
    if (egress_programs_[i]) {
      del_program(i, ProgramType::EGRESS);
    }
  }
}

Cube::~Cube() {
  id_generator_.release(id_);
  for (auto &it : ports_by_name_) {
    it.second->set_peer("");
  }
}

uint32_t Cube::get_id() const {
  return id_;
}

uint16_t Cube::get_index(ProgramType type) const {
  switch (type) {
  case ProgramType::INGRESS:
    return ingress_index_;
  case ProgramType::EGRESS:
    return egress_index_;
  }
  return 0;
}

uint16_t Cube::allocate_port_id() {
  if (free_ports_.size() < 1) {
    logger->error("there are not free ports in cube '{0}'", name_);
    throw std::runtime_error("No free ports");
  }
  uint16_t p = *free_ports_.begin();
  free_ports_.erase(p);
  return p;
}

void Cube::release_port_id(uint16_t id) {
  free_ports_.insert(id);
}

std::shared_ptr<PortIface> Cube::add_port(const std::string &name) {
  std::lock_guard<std::mutex> cube_guard(cube_mutex_);
  if (ports_by_name_.count(name) != 0) {
    throw std::runtime_error("Port " + name + " already exists");
  }
  auto id = allocate_port_id();

  std::shared_ptr<PortIface> port;

  switch (type_) {
  case CubeType::TC:
    port = std::make_shared<PortTC>(*this, name, id);
    break;
  case CubeType::XDP_SKB:
  case CubeType::XDP_DRV:
    port = std::make_shared<PortXDP>(*this, name, id);
    break;
  }

  ports_by_name_.emplace(name, port);
  ports_by_index_.emplace(id, port);
  return std::move(port);
}

void Cube::remove_port(const std::string &name) {
  if (ports_by_name_.count(name) == 0) {
    throw std::runtime_error("Port " + name + " does not exist");
  }

  // set_peer("") has to be done before acquiring cube_guard because
  // a dead lock is possible when destroying the port:
  // lock(cube_mutex_) -> ~Port::Port() -> Port::set_peer() ->
  // ServiceController::set_{tc,xdp}_port_peer() ->
  // Port::unconnect() -> Cube::update_forwarding_table -> lock(cube_mutex_)

  ports_by_name_.at(name)->set_peer("");

  std::lock_guard<std::mutex> cube_guard(cube_mutex_);
  auto index = ports_by_name_.at(name)->index();
  ports_by_name_.erase(name);
  ports_by_index_.erase(index);
  release_port_id(index);
}

std::shared_ptr<PortIface> Cube::get_port(const std::string &name) {
  std::lock_guard<std::mutex> cube_guard(cube_mutex_);
  if (ports_by_name_.count(name) == 0) {
    throw std::runtime_error("Port " + name + " does not exist");
  }
  return ports_by_name_.at(name);
}

CubeType Cube::get_type() const {
  return type_;
}

void Cube::update_forwarding_table(int index, int value) {
  std::lock_guard<std::mutex> cube_guard(cube_mutex_);
  if (forward_chain_)  // is the forward chain still active?
    forward_chain_->update_value(index, value);
}

const std::string Cube::get_name() const {
  return name_;
}

const Guid &Cube::uuid() const {
  return uuid_;
}

int Cube::get_table_fd(const std::string &table_name, int index,
                       ProgramType type) {
  // TODO: Improve bcc api to do it
  switch (type) {
  case ProgramType::INGRESS:
    return ingress_programs_[index]->get_table(table_name).fd();
  case ProgramType::EGRESS:
    return egress_programs_[index]->get_table(table_name).fd();
  }
}

void Cube::reload(const std::string &code, int index, ProgramType type) {
  std::lock_guard<std::mutex> cube_guard(cube_mutex_);
  return do_reload(code, index, type);
}

void Cube::do_reload(const std::string &code, int index, ProgramType type) {
#ifdef LOG_COMPILATION_TIME
  auto start = std::chrono::high_resolution_clock::now();
#endif

  std::array<std::unique_ptr<ebpf::BPF>, _POLYCUBE_MAX_BPF_PROGRAMS> *programs;
  std::array<std::string, _POLYCUBE_MAX_BPF_PROGRAMS> *code_;
  int fd;

  switch (type) {
  case ProgramType::INGRESS:
    programs = &ingress_programs_;
    code_ = &ingress_code_;
    break;
  case ProgramType::EGRESS:
    programs = &egress_programs_;
    code_ = &egress_code_;
    break;
  default:
    throw std::runtime_error("Bad program type");
  }

  if (index >= programs->size()) {
    throw std::runtime_error("Invalid ebpf program index");
  }

  if (programs->at(index) == nullptr) {
    throw std::runtime_error("ebpf does not exist");
  }

  // create new ebpf program, telling to steal the maps of this program
  std::unique_lock<std::mutex> bcc_guard(bcc_mutex);
  std::unique_ptr<ebpf::BPF> new_bpf_program = std::unique_ptr<ebpf::BPF>(
      new ebpf::BPF(0, nullptr, false, name_, &*programs->at(index)));

  bcc_guard.unlock();
  compile(*new_bpf_program, code, index, type);
  fd = load(*new_bpf_program, type);

  switch (type) {
  case ProgramType::INGRESS:
    ingress_programs_table_->update_value(index, fd);
    // update patch panel if program is master
    if (index == 0) {
      ingress_fd_ = fd;
      patch_panel_ingress_.update(ingress_index_, ingress_fd_);
    }
    break;
  case ProgramType::EGRESS:
    egress_programs_table_->update_value(index, fd);
    // update patch panel if program is master
    if (index == 0) {
      egress_fd_ = fd;
      patch_panel_egress_.update(egress_index_, egress_fd_);
    }
    break;
  }

  unload(*programs->at(index), type);
  (*programs)[index] = std::move(new_bpf_program);
  // update last used code
  (*code_)[index] = code;

#ifdef LOG_COMPILATION_TIME
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed_seconds = end - start;
  logger->info("+reload: {0}s", elapsed_seconds.count());
#endif
}

int Cube::add_program(const std::string &code, int index, ProgramType type) {
  std::lock_guard<std::mutex> cube_guard(cube_mutex_);

  std::array<std::unique_ptr<ebpf::BPF>, _POLYCUBE_MAX_BPF_PROGRAMS> *programs;
  std::array<std::string, _POLYCUBE_MAX_BPF_PROGRAMS> *code_;

  switch (type) {
  case ProgramType::INGRESS:
    programs = &ingress_programs_;
    code_ = &ingress_code_;
    break;
  case ProgramType::EGRESS:
    programs = &egress_programs_;
    code_ = &egress_code_;
    break;
  default:
    throw std::runtime_error("Bad program type");
  }

  // check index boundaries
  if (index < 0 || index >= programs->size()) {
    throw std::runtime_error("Invalid ebpf program index");
  }

  if (index != -1) {
    if (programs->at(index) != nullptr) {
      throw std::runtime_error("Program is not empty");
    }
  } else {
    int i;
    for (i = 0; i < programs->size(); i++) {
      if (programs->at(i) == nullptr) {
        index = i;
        break;
      }
    }

    if (i == programs->size()) {
      throw std::runtime_error("Maximum number of programs reached");
    }
  }

  std::unique_lock<std::mutex> bcc_guard(bcc_mutex);
  // load and add this program to the list
  int fd_;
  (*programs)[index] =
      std::unique_ptr<ebpf::BPF>(new ebpf::BPF(0, nullptr, false, name_));

  bcc_guard.unlock();
  compile(*programs->at(index), code, index, type);
  fd_ = load(*programs->at(index), type);
  bcc_guard.lock();

  // TODO: this switch could also be removed creating variables in the switch
  // above
  switch (type) {
  case ProgramType::INGRESS:
    ingress_programs_table_->update_value(index, fd_);
    if (index == 0) {
      ingress_fd_ = fd_;
      if (ingress_index_) {
        // already registed in patch panel, just update
        patch_panel_ingress_.update(ingress_index_, ingress_fd_);
      } else {
        // register in patch panel
        ingress_index_ = patch_panel_ingress_.add(ingress_fd_);
      }
    }
    break;
  case ProgramType::EGRESS:
    egress_programs_table_->update_value(index, fd_);
    if (index == 0) {
      egress_fd_ = fd_;
      if (egress_index_) {
        // already registed in patch panel, just update
        patch_panel_egress_.update(egress_index_, egress_fd_);
      } else {
        // register in patch panel
        egress_index_ = patch_panel_egress_.add(egress_fd_);
      }
      break;
    }
  }

  (*code_)[index] = code;

  return index;
}

void Cube::del_program(int index, ProgramType type) {
  std::lock_guard<std::mutex> cube_guard(cube_mutex_);

  std::array<std::unique_ptr<ebpf::BPF>, _POLYCUBE_MAX_BPF_PROGRAMS> *programs;
  std::array<std::string, _POLYCUBE_MAX_BPF_PROGRAMS> *code_;
  ebpf::BPFProgTable *programs_table;

  switch (type) {
  case ProgramType::INGRESS:
    programs = &ingress_programs_;
    code_ = &ingress_code_;
    programs_table = ingress_programs_table_.get();  // TODO: is this valid?
    break;
  case ProgramType::EGRESS:
    programs = &egress_programs_;
    code_ = &egress_code_;
    programs_table = egress_programs_table_.get();  // TODO: is this valid?
    break;
  default:
    throw std::runtime_error("Bad program type");
  }

  if (index < 0 || index >= programs->size()) {
    logger->error("Invalid ebpf program index");
    throw std::runtime_error("Invalid ebpf program index");
  }

  if (programs->at(index) == nullptr) {
    throw std::runtime_error("ebpf program does not exist");
  }

  std::unique_lock<std::mutex> bcc_guard(bcc_mutex);
  programs_table->remove_value(index);
  bcc_mutex.unlock();
  unload(*programs->at(index), type);
  bcc_guard.unlock();
  (*programs)[index] = nullptr;
  code_->at(index).clear();
}

void Cube::set_log_level(LogLevel level) {
  std::lock_guard<std::mutex> cube_guard(cube_mutex_);
  if (level_ == level)
    return;

  level_ = level;
  for (int i = 0; i < ingress_programs_.size(); i++) {
    if (ingress_programs_[i]) {
      do_reload(ingress_code_[i], i, ProgramType::INGRESS);
    }
  }

  for (int i = 0; i < egress_programs_.size(); i++) {
    if (egress_programs_[i]) {
      do_reload(egress_code_[i], i, ProgramType::EGRESS);
    }
  }
}

LogLevel Cube::get_log_level() const {
  return level_;
}

json Cube::toJson(bool include_ports) const {
  json j = {
      {"name", name_}, {"uuid", uuid_.str()}, {"service", service_name_},
      //{"type", type_},
  };

  if (include_ports) {
    json ports_json = json::array();

    for (auto &it : ports_by_name_) {
      json port_json = json::object();
      port_json["name"] = it.second->name();
      auto peer = it.second->peer();

      if (peer.size() > 0) {
        port_json["peer"] = peer;
      }

      ports_json += port_json;
    }

    if (ports_json.size() > 0) {
      j["ports"] = ports_json;
    }
  }

  return j;
}

IDGenerator Cube::id_generator_(Node::_POLYCUBE_MAX_NODES - 2);

const std::string Cube::MASTER_CODE = R"(
// table used to save ports to endpoint relation
BPF_TABLE_SHARED("array", int, u32, forward_chain_, _POLYCUBE_MAX_PORTS);

// tables to save file descriptor of ingress and egress programs
BPF_TABLE_SHARED("prog", int, int, ingress_programs, _POLYCUBE_MAX_BPF_PROGRAMS);
BPF_TABLE_SHARED("prog", int, int, egress_programs, _POLYCUBE_MAX_BPF_PROGRAMS);
)";

const std::string Cube::CUBE_H = R"(
#define KBUILD_MODNAME "MOD_NAME"
#include <bcc/helpers.h>
#include <bcc/proto.h>
#include <uapi/linux/pkt_cls.h>

#include <uapi/linux/bpf.h>

#define CONTROLLER_MODULE_INDEX (_POLYCUBE_MAX_NODES - 1)

// maps definitions, same as in master program but "extern"
BPF_TABLE("extern", int, u32, forward_chain_, _POLYCUBE_MAX_PORTS);
BPF_TABLE("extern", int, int, ingress_programs, _POLYCUBE_MAX_BPF_PROGRAMS);
BPF_TABLE("extern", int, int, egress_programs, _POLYCUBE_MAX_BPF_PROGRAMS);

enum {
  RX_OK,
  RX_REDIRECT,
  RX_DROP,
  RX_RECIRCULATE,
  RX_CONTROLLER,
  RX_ERROR,
};

struct pkt_metadata {
  u16 cube_id;      //__attribute__((deprecated)) // use CUBE_ID instead
  u16 in_port;      // The interface on which a packet was received.
  u32 packet_len;   //__attribute__((deprecated)) // Use ctx->len

  // used to send data to controller
  u16 reason;
  u32 md[3];
} __attribute__((packed));

// cube must implement this function to attach to the networking stack
static __always_inline
int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md);

static __always_inline
int pcn_pkt_redirect(struct CTXTYPE *skb,
                     struct pkt_metadata *md, u32 port);

static __always_inline
int pcn_pkt_drop(struct CTXTYPE *skb, struct pkt_metadata *md);

static __always_inline
int pcn_pkt_controller(struct CTXTYPE *skb, struct pkt_metadata *md,
                       u16 reason);

static __always_inline
int pcn_pkt_controller_with_metadata(struct CTXTYPE *skb,
                                     struct pkt_metadata *md,
                                     u16 reason,
                                     u32 metadata[3]);

static __always_inline
void call_ingress_program(struct CTXTYPE *skb, int index) {
  ingress_programs.call(skb, index);
}

static __always_inline
void call_egress_program(struct CTXTYPE *skb, int index) {
  egress_programs.call(skb, index);
}

/* checksum related */

// those functions have different implementations for XDP and TC
static __always_inline
int pcn_l3_csum_replace(struct CTXTYPE *ctx, u32 csum_offset,
                        u32 old_value, u32 new_value, u32 flags);

static __always_inline
int pcn_l4_csum_replace(struct CTXTYPE *ctx, u32 csum_offset,
                        u32 old_value, u32 new_value, u32 flags);

static __always_inline
__wsum pcn_csum_diff(__be32 *from, u32 from_size, __be32 *to,
                     u32 to_size, __wsum seed);

)";

}  // namespace polycubed
}  // namespace polycube
