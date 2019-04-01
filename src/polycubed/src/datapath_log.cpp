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

#include "datapath_log.h"

#include <regex>

namespace polycube {
namespace polycubed {

static const std::string LOG_BUFFER = R"(
  struct log_table_t {
  int key;
  u32 leaf;
  /* map.perf_submit(ctx, data, data_size) */
  int (*perf_submit) (void *, void *, u32);
  int (*perf_submit_skb) (void *, u32, void *, u32);
  u32 data[0];
};
__attribute__((section("maps/perf_output")))
struct log_table_t log_buffer;
__attribute__((section("maps/export")))
struct log_table_t __log_buffer;
)";

static const std::string REPLACE_BASE = R"(
do {
#if GET_LEVEL($1) >= LOG_LEVEL
  LOG_STRUCT($1);
  log_buffer.perf_submit(GET_CTX($1), &msg_struct, sizeof(msg_struct));
#endif
} while(0);
)";

static const std::string REPLACE_BASE_PKT = R"(
do {
#if GET_LEVEL($1) >= LOG_LEVEL
  LOG_STRUCT_PKT($1);
  log_buffer.perf_submit_skb(GET_CTX($1), msg_struct.len, &msg_struct, sizeof(msg_struct));
#endif
} while(0);
)";

static const std::string BASE_CODE = R"(
/*** logging related stuff ***/

// perf ring buffer used for debug purposes
struct log_table_t { \
  int key;                                            \
  u32 leaf;                                           \
  /* map.perf_submit(ctx, data, data_size) */         \
  int (*perf_submit) (void *, void *, u32);           \
  int (*perf_submit_skb) (void *, u32, void *, u32);  \
  u32 data[0];                                        \
};                                                    \
__attribute__((section("maps/extern")))               \
struct log_table_t log_buffer;

// DON'T use an enum, it breaks conditional code compilation
#define LOG_TRACE 0
#define LOG_DEBUG 1
#define LOG_INFO 2
#define LOG_WARN 3
#define LOG_ERR 4
#define LOG_CRITICAL 5
#define LOG_OFF 6

#define GET_CTX(_ctx, _level, ...) _ctx
#define GET_LEVEL(_ctx, _level, ...) _level

// for some reason the verifier gives an error if CUBE_ID
// is placed directly into the structure intilization

#define LOG_STRUCT(_ctx, _level, _msg, ...)                 \
  uint16_t __id = CUBE_ID;                                  \
  struct __attribute__((__packed__)) LogMsg {               \
    uint16_t type;                                          \
    uint16_t id;                                            \
    uint16_t level;                                         \
    uint16_t len;                                           \
    uint64_t args[4];                                       \
    char msg[sizeof(_msg)];                                 \
  } msg_struct = {0, __id, _level, sizeof(_msg),            \
      {__VA_ARGS__}, _msg};

#ifdef POLYCUBE_XDP
#define LOG_STRUCT_PKT(_ctx, _level)                        \
  uint16_t __id = CUBE_ID;                                  \
  uint16_t len = (uint16_t)(ctx->data_end - ctx->data);     \
  struct __attribute__((__packed__)) LogMsg {               \
    uint16_t type;                                          \
    uint16_t id;                                            \
    uint16_t level;                                         \
    uint16_t len;                                           \
    uint64_t args[4];                                       \
  } msg_struct = {1, __id, _level, len, {0, 0, 0, 0}};
#else
#define LOG_STRUCT_PKT(_ctx, _level)                        \
  uint16_t __id = CUBE_ID;                                  \
  uint16_t len = ctx->len;                                  \
  struct __attribute__((__packed__)) LogMsg {               \
    uint16_t type;                                          \
    uint16_t id;                                            \
    uint16_t level;                                         \
    uint16_t len;                                           \
    uint64_t args[4];                                       \
  } msg_struct = {1, __id, _level, len, {0, 0, 0, 0}};
#endif
)";

void DatapathLog::call_back_proxy(void *cb_cookie, void *data, int data_size) {
  LogMsg *log_msg = static_cast<LogMsg *>(data);
  // spdlog::get("polycubed")->debug("call_back_proxy");

  DatapathLog *c = static_cast<DatapathLog *>(cb_cookie);
  if (c == nullptr)
    throw std::runtime_error("Bad datapathlog");

  std::lock_guard<std::mutex> guard(c->cbs_mutex_);

  if (c->cbs_.count(log_msg->cube_id) > 0) {
    auto cb = c->cbs_.at(log_msg->cube_id);
    cb(log_msg);
  } else {
    spdlog::get("polycubed")->warn("log message for non existing module");
  }
}

DatapathLog &DatapathLog::get_instance() {
  static DatapathLog instance;
  return instance;
}

DatapathLog::DatapathLog() : logger(spdlog::get("polycubed")) {
  auto res = perf_buffer_.init(LOG_BUFFER);
  if (res.code() != 0) {
    logger->error("impossible to load log buffer: {0}", res.msg());
    throw std::runtime_error("Error loading log buffer");
  }

  res = perf_buffer_.open_perf_buffer("log_buffer", call_back_proxy, nullptr,
                                      this);
  if (res.code() != 0) {
    logger->error("Cannot open perf ring buffer for controller: {0}",
                  res.msg());
    throw std::runtime_error("Error opening perf buffer: " + res.msg());
  }

  start();
}

DatapathLog::~DatapathLog() {
  stop();
}

void DatapathLog::register_cb(int id, const log_msg_cb &cb) {
  std::lock_guard<std::mutex> guard(cbs_mutex_);
  cbs_.insert(std::pair<int, const log_msg_cb &>(id, cb));
}

void DatapathLog::unregister_cb(int id) {
  std::lock_guard<std::mutex> guard(cbs_mutex_);
  cbs_.erase(id);
}

void DatapathLog::start() {
  // create a thread that polls the perf ring buffer
  auto f = [&]() -> void {
    stop_ = false;
    while (!stop_) {
      perf_buffer_.poll_perf_buffer("log_buffer", 500);
    }

    // TODO: this causes a segmentation fault
    //  logger->debug("controller: stopping");
  };

  std::unique_ptr<std::thread> uptr(new std::thread(f));
  dbg_thread_ = std::move(uptr);
}

void DatapathLog::stop() {
  stop_ = true;
  if (dbg_thread_) {
    dbg_thread_->join();
  }
}

std::string DatapathLog::replace_string(std::string &subject,
                                        const std::string &search,
                                        const std::string &replace) {
  size_t start_pos = 0;
  while ((start_pos = subject.find(search, start_pos)) != std::string::npos) {
    subject.replace(start_pos, search.length(), replace);
    start_pos +=
        replace.length();  // Handles case where 'to' is a substring of 'from'
  }
  return subject;
}

// matches all C++ comments
// https://stackoverflow.com/questions/36454069/how-to-remove-c-style-comments-from-code
static std::regex regComments(
    R"***((?:\/\/(?:\\\n|[^\n])*)|(?:\/\*[\s\S]*?\*\/)|((?:R"([^(\\\s]{0,16})\([^)]*\)\2")|(?:@"[^"]*?")|(?:"(?:\?\?'|\\\\|\\"|\\\n|[^"])*?")|(?:'(?:\\\\|\\'|\\\n|[^'])*?')))***");

// matches all calls to pcn_log(ctx, ...)
static std::regex regN(R"***(pcn_log\s*\(([\s\S]+?)\)\s*;)***");
static std::regex regNewLine(R"***(/(\r\n)+|\r+|\n+|\t+/i)***");
static std::regex regAddSpaces(R"***( +)***");

static std::regex regNPkt(R"***(pcn_pkt_log\s*\(([\s\S]+?)\)\s*;)***");

std::string DatapathLog::dp_callback(const std::smatch &m) {
  std::string match = std::regex_replace(m.str(1), regNewLine, "");
  match = std::regex_replace(match, regAddSpaces, " ");
  std::string new_string = std::string(REPLACE_BASE);
  new_string = DatapathLog::replace_string(new_string, "$1", match);
  return new_string;
}

std::string DatapathLog::dp_callback_pkt(const std::smatch &m) {
  std::string match = std::regex_replace(m.str(1), regNewLine, "");
  match = std::regex_replace(match, regAddSpaces, " ");
  std::string new_string = std::string(REPLACE_BASE_PKT);
  new_string = DatapathLog::replace_string(new_string, "$1", match);
  return new_string;
}

std::string DatapathLog::parse_log(const std::string &code) {
  // remove all comments from the code before going on
  auto code1 = std::regex_replace(code, regComments, "$1");
  auto code2 = std::regex_replace_cb(code1, regN, DatapathLog::dp_callback);
  auto code3 =
      std::regex_replace_cb(code2, regNPkt, DatapathLog::dp_callback_pkt);
  return BASE_CODE + code3;
}

}  // namespace polycubed
}  // namespace polycube
