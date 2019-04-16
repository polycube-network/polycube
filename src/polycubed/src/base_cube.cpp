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

#include "base_cube.h"

#include "polycube/common.h"

namespace polycube {
namespace polycubed {

std::vector<std::string> BaseCube::cflags = {
    std::string("-D_POLYCUBE_MAX_NODES=") +
        std::to_string(PatchPanel::_POLYCUBE_MAX_NODES),
    std::string("-D_POLYCUBE_MAX_BPF_PROGRAMS=") +
        std::to_string(_POLYCUBE_MAX_BPF_PROGRAMS),
    std::string("-D_POLYCUBE_MAX_PORTS=") + std::to_string(_POLYCUBE_MAX_PORTS),
};

BaseCube::BaseCube(const std::string &name, const std::string &service_name,
                   const std::string &master_code,
                   PatchPanel &patch_panel_ingress,
                   PatchPanel &patch_panel_egress, LogLevel level,
                   CubeType type)
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
  master_program_->init(BASECUBE_MASTER_CODE + master_code, cflags);

  // get references to those maps
  auto ingress_ = master_program_->get_prog_table("ingress_programs");
  ingress_programs_table_ =
      std::unique_ptr<ebpf::BPFProgTable>(new ebpf::BPFProgTable(ingress_));

  auto egress_ = master_program_->get_prog_table("egress_programs");
  egress_programs_table_ =
      std::unique_ptr<ebpf::BPFProgTable>(new ebpf::BPFProgTable(egress_));
}

void BaseCube::init(const std::vector<std::string> &ingress_code,
                    const std::vector<std::string> &egress_code) {
  for (int i = 0; i < ingress_code.size(); i++) {
    add_program(ingress_code[i], i, ProgramType::INGRESS);
  }

  for (int i = 0; i < egress_code.size(); i++) {
    add_program(egress_code[i], i, ProgramType::EGRESS);
  }
}

void BaseCube::uninit() {
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

BaseCube::~BaseCube() {
  id_generator_.release(id_);
}

uint32_t BaseCube::get_id() const {
  return id_;
}

uint16_t BaseCube::get_index(ProgramType type) const {
  switch (type) {
  case ProgramType::INGRESS:
    return ingress_index_;
  case ProgramType::EGRESS:
    return egress_index_;
  }
  return 0;
}

CubeType BaseCube::get_type() const {
  return type_;
}

const std::string BaseCube::get_name() const {
  return name_;
}

const std::string BaseCube::get_service_name() const {
  return service_name_;
}

const Guid &BaseCube::uuid() const {
  return uuid_;
}

int BaseCube::get_table_fd(const std::string &table_name, int index,
                           ProgramType type) {
  // TODO: Improve bcc api to do it
  switch (type) {
  case ProgramType::INGRESS:
    return ingress_programs_[index]->get_table(table_name).fd();
  case ProgramType::EGRESS:
    return egress_programs_[index]->get_table(table_name).fd();
  }
}

void BaseCube::reload(const std::string &code, int index, ProgramType type) {
  std::lock_guard<std::mutex> cube_guard(cube_mutex_);
  return do_reload(code, index, type);
}

void BaseCube::do_reload(const std::string &code, int index, ProgramType type) {
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
  bcc_guard.lock();
  (*programs)[index] = std::move(new_bpf_program);
  // update last used code
  (*code_)[index] = code;
}

void BaseCube::reload_all() {
  std::lock_guard<std::mutex> cube_guard(cube_mutex_);
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

int BaseCube::add_program(const std::string &code, int index,
                          ProgramType type) {
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

void BaseCube::del_program(int index, ProgramType type) {
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

  programs_table->remove_value(index);
  unload(*programs->at(index), type);
  std::unique_lock<std::mutex> bcc_guard(bcc_mutex);
  programs->at(index).reset();
  code_->at(index).clear();
}

std::string BaseCube::get_wrapper_code() {
  return BASECUBE_WRAPPER;
}

void BaseCube::set_log_level(LogLevel level) {
  if (level_ == level)
    return;

  // change log level in dataplane
  log_level_cb_(level);

  level_ = level;
  reload_all();
}

LogLevel BaseCube::get_log_level() const {
  return level_;
}

void BaseCube::set_log_level_cb(const polycube::service::set_log_level_cb &cb) {
  log_level_cb_ = cb;
}

void BaseCube::set_conf(const nlohmann::json &conf) {
  if (conf.count("loglevel")) {
    set_log_level(stringLogLevel(conf.at("loglevel").get<std::string>()));
  }
}

nlohmann::json BaseCube::to_json() const {
  nlohmann::json j;

  j["name"] = name_;
  j["uuid"] = uuid_.str();
  j["service-name"] = service_name_;
  j["type"] = cube_type_to_string(type_);
  j["loglevel"] = logLevelString(level_);

  return j;
}

IDGenerator BaseCube::id_generator_(PatchPanel::_POLYCUBE_MAX_NODES - 2);

const std::string BaseCube::BASECUBE_MASTER_CODE = R"(
// tables to save file descriptor of ingress and egress programs
BPF_TABLE_SHARED("prog", int, int, ingress_programs, _POLYCUBE_MAX_BPF_PROGRAMS);
BPF_TABLE_SHARED("prog", int, int, egress_programs, _POLYCUBE_MAX_BPF_PROGRAMS);
)";

const std::string BaseCube::BASECUBE_WRAPPER = R"(
#define KBUILD_MODNAME "MOD_NAME"
#include <bcc/helpers.h>
#include <bcc/proto.h>
#include <uapi/linux/pkt_cls.h>

#include <uapi/linux/bpf.h>

#define CONTROLLER_MODULE_INDEX (_POLYCUBE_MAX_NODES - 1)

// maps definitions, same as in master program but "extern"
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
