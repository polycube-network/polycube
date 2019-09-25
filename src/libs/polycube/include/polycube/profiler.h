/*
 * Copyright 2019 The Polycube Authors
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

#ifdef PROFILER

#ifndef POLYCUBE_PROFILER_H
#define POLYCUBE_PROFILER_H

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#define WARM_UP 100

#define PROFILER_FILENAME "/var/log/polycube/profile"
#define PROFILER_EXTENSION ".log"

#define CHECKPOINT(...) \
  Profiler::getInstance().tick(__FILE__, __LINE__, ##__VA_ARGS__);
#define STOREPOINT(...) \
  { CHECKPOINT(__VA_ARGS__) Profiler::getInstance().store(); }

/***
 * Profiler Singleton header class to measure execution time of certain
 * functions
 */
class Profiler {
 private:
  /// Private class constructor to avoid instantiation, includes a warm-up phase
  /// for the timer and cache loading
  Profiler();

  /// Support data structure <file,line><checkpoint_name,timestamp>
  std::vector<std::pair<
      std::pair<const char *, unsigned>,
      std::pair<const char *, std::chrono::high_resolution_clock::time_point>>>
      results;

  /// Final filename calculated at execution time
  std::string output_file;

 public:
  /**
   * Function to be called before the `objective function` execution to store
   * the initial timestamp
   * @param file: the name of the file containing the checkpoint
   * @param line: the line of code where the checkpoint is
   * @param checkpoint_name: user defined checkpoint name
   */
  void tick(const char *file = "", unsigned line = 0,
            const char *checkpoint_name = "");

  /// Function called to store gathered data into a file
  void store();

  /// Singleton implementation, method to return the instance
  static Profiler &getInstance() {
    static Profiler instance;
    return instance;
  }
};

inline Profiler::Profiler() {
  auto now = time(nullptr);
  auto ltm = localtime(&now);

  // Setting output file file according to template: <filename>_YYYYMMDD_HHMMSS
  output_file = dynamic_cast<std::ostringstream &>(
                    std::ostringstream().seekp(0)
                    << PROFILER_FILENAME << "_" << std::setw(4)
                    << 1900 + ltm->tm_year << std::setfill('0') << std::setw(2)
                    << 1 + ltm->tm_mon << std::setfill('0') << std::setw(2)
                    << ltm->tm_mday << "_" << std::setfill('0') << std::setw(2)
                    << ltm->tm_hour << std::setfill('0') << std::setw(2)
                    << ltm->tm_min << std::setfill('0') << std::setw(2)
                    << ltm->tm_sec << PROFILER_EXTENSION)
                    .str();

  // Creating file and changing permissions to make it writable also by normal
  // user. This is extremely necessary since the user could insert a CHECKPOINT
  // in some part of the code which is executed by root. If the output file is
  // created in that moment, the ones inserted in a non-root part of the code
  // would not be stored due to lower permissions.
  std::ofstream os;
  os.open(output_file, std::ios::out);
  os << "File, Line, Checkpoint Name, Timestamp" << std::endl;
  os.close();
  chmod(output_file.c_str(), S_IRWXO | S_IRWXG | S_IRWXU);

  // Warm up phase for timer and cache
  for (int i = 0; i < WARM_UP; i++) {
    tick();
  }
  results.clear();
}

inline void Profiler::store() {
  std::ofstream os;
  os.open(output_file, std::ios::app);
  std::for_each(results.begin(), results.end(), [&os](auto &res) -> void {
    os << res.first.first << "," << res.first.second << "," << res.second.first
       << "," << res.second.second.time_since_epoch().count() << std::endl;
  });
  os.close();
  results.clear();
}

inline void Profiler::tick(const char *filename, unsigned line_number,
                           const char *checkpoint_name) {
  results.emplace_back(std::make_pair(
      std::make_pair(filename, line_number),
      std::make_pair(checkpoint_name,
                     std::chrono::high_resolution_clock::now())));
}

#endif  // POLYCUBE_PROFILER_H

#else

#define CHECKPOINT(...)
#define STOREPOINT(...)

#endif  // PROFILER
