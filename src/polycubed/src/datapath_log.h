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

#include <functional>
#include <thread>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <regex>

#include <api/BPF.h>
#include <spdlog/spdlog.h>

#include "polycube/services/cube_factory.h"

using polycube::service::LogMsg;
using polycube::service::log_msg_cb;

namespace polycube {
namespace polycubed {

class DatapathLog {
 public:
  static DatapathLog &get_instance();
  ~DatapathLog();
  void register_cb(int id, const log_msg_cb &cb);
  void unregister_cb(int id);

  void start();
  void stop();

  static void call_back_proxy(void *cb_cookie, void *data, int data_size);
  static std::string replace_string(std::string& subject, const std::string& search, const std::string& replace);
  static std::string dp_callback(const std::smatch& m);
  static std::string dp_callback_pkt(const std::smatch& m);

  // replaces all the log calls to the code that does it
  std::string parse_log(const std::string &code);

 private:
  DatapathLog();
  std::unique_ptr<std::thread> dbg_thread_;
  ebpf::BPF perf_buffer_;
  std::map<uint32_t, const log_msg_cb &> cbs_;
  std::mutex cbs_mutex_; // protects the cbs_ container
  std::shared_ptr<spdlog::logger> logger;
  bool stop_;
  std::string base_code_;

};

}  // namespace polycubed
}  // namespace polycube

// Based on solution proposed in: https://stackoverflow.com/questions/22617209/regex-replace-with-callback-in-c11
namespace std
{

template<class BidirIt, class Traits, class CharT, class UnaryFunction>
std::basic_string<CharT> regex_replace_cb(BidirIt first, BidirIt last,
    const std::basic_regex<CharT,Traits>& re, UnaryFunction f)
{
    std::basic_string<CharT> s;

    typename std::match_results<BidirIt>::difference_type
        positionOfLastMatch = 0;
    auto endOfLastMatch = first;

    auto callback = [&](const std::match_results<BidirIt>& match)
    {
        auto positionOfThisMatch = match.position(0);
        auto diff = positionOfThisMatch - positionOfLastMatch;

        auto startOfThisMatch = endOfLastMatch;
        std::advance(startOfThisMatch, diff);

        s.append(endOfLastMatch, startOfThisMatch);
        s.append(f(match));

        auto lengthOfMatch = match.length(0);

        positionOfLastMatch = positionOfThisMatch + lengthOfMatch;

        endOfLastMatch = startOfThisMatch;
        std::advance(endOfLastMatch, lengthOfMatch);
    };

    std::regex_iterator<BidirIt> begin(first, last, re), end;
    std::for_each(begin, end, callback);

    s.append(endOfLastMatch, last);

    return s;
}

template<class Traits, class CharT, class UnaryFunction>
std::string regex_replace_cb(const std::string& s,
    const std::basic_regex<CharT,Traits>& re, UnaryFunction f)
{
    return regex_replace_cb(s.cbegin(), s.cend(), re, f);
}

} // namespace std
