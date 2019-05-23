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

#include "ns.h"

#include <iostream>
#include <sstream>
#include <cstring>

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sched.h>
#include <sys/mount.h>
#include <linux/limits.h>

#include <sys/types.h>
#include <sys/syscall.h>

#include <netlink/netlink.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>
#include <netlink/route/qdisc.h>
#include <netlink/socket.h>

#include <linux/net_namespace.h>

#define NETNS_RUN_DIR "/var/run/netns"

namespace polycube::polycubed {

struct Namespace::Event {
  std::function<void()> f;
  std::promise<void> barrier;
};

std::unique_ptr<std::thread> Namespace::executor;
std::condition_variable Namespace::cv;
std::mutex Namespace::mutex;
std::queue<Namespace::Event *> Namespace::queue;
bool Namespace::finished(false);
std::atomic<int> Namespace::count(0);


Namespace Namespace::create(const std::string &name) {
  if (count.fetch_add(1) == 0) {
    start();
  }

  try {
    int fd;

    auto f = [&]() {
      create_ns(name);
      char netns_path[PATH_MAX];
      ::snprintf(netns_path, sizeof(netns_path), "%s/%s",
                 NETNS_RUN_DIR, name.c_str());
      fd = ::open(netns_path, O_RDONLY|O_EXCL, 0);
      if (fd < 0) {
        throw std::runtime_error(std::string("Error opening namespace: ") +
                                 std::strerror(errno));
      }
    };

    execute_in_worker(f);
    return Namespace(name, fd);
  } catch (...) {
    if (count.fetch_sub(1) == 1) {
      stop();
    }
    throw;
  }
}

Namespace Namespace::open(const std::string &name) {
  if (count.fetch_add(1) == 0) {
    start();
  }

  try {
    int fd;

    auto f = [&]() {
      char netns_path[PATH_MAX];
      ::snprintf(netns_path, sizeof(netns_path), "%s/%s", NETNS_RUN_DIR,
                 name.c_str());
      fd = ::open(netns_path, O_RDONLY|O_EXCL, 0);
      if (fd < 0) {
        throw std::runtime_error(std::string("Error opening namespace: ") +
                                 std::strerror(errno));
      }
    };

    execute_in_worker(f);
    return Namespace(name, fd);
  } catch (...) {
    if (count.fetch_sub(1) == 1) {
      stop();
    }
    throw;
  }
}

Namespace::Namespace(const std::string &name, int fd) : name_(name), fd_(fd){}

Namespace::~Namespace() {
  close(fd_);

  if (count.fetch_sub(1) == 1) {
    stop();
  }
}

void Namespace::execute(std::function<void()> f) {
  std::function<void()> f_ = [&]() {
    if (setns(fd_, CLONE_NEWNET)) {
      throw std::runtime_error(std::string("setns error: ") +
                               std::strerror(errno));
    }
    f();
  };

  execute_in_worker(f_);
}

void Namespace::remove() {
  char netns_path[PATH_MAX];
  snprintf(netns_path, sizeof(netns_path), "%s/%s", NETNS_RUN_DIR,
           name_.c_str());
  ::close(fd_);

  ::umount2(netns_path, MNT_DETACH);
  if (unlink(netns_path) < 0) {
    throw std::runtime_error(std::string("umount error : ") +
                             std::strerror(errno));
  }
}


void Namespace::set_id(int id) {
  int err;

  struct rtnl_link *link;
  struct nl_cache *cache;
  struct nl_sock *sock;

  sock = nl_socket_alloc();
  if ((err = nl_connect(sock, NETLINK_ROUTE)) < 0) {
    throw std::runtime_error(std::string("Unable to connect socket: ") +
                             nl_geterror(err));
  }

  if ((err = rtnl_link_alloc_cache(sock, AF_UNSPEC, &cache)) < 0) {
    throw std::runtime_error(std::string("Unable to allocate cache: ") +
                             nl_geterror(err));
  }

  struct nl_msg *msg;
  struct nlmsghdr *hdr;

  struct nlattr *opts;

  if (!(msg = nlmsg_alloc())) {
    throw std::runtime_error("Unable allocate nlmsg");
  }

  struct rtgenmsg t;
  t.rtgen_family = AF_UNSPEC;

  hdr = nlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, RTM_NEWNSID, sizeof(t),
                  NLM_F_REQUEST);
  memcpy(nlmsg_data(hdr), &t, sizeof(t));

  NLA_PUT_U32(msg, NETNSA_FD, fd_);
  NLA_PUT_U32(msg, NETNSA_NSID, id);

  if(nl_send_sync(sock, msg)) {
    throw std::runtime_error("nl_send_sync failed");
  }

  // TODO: read response from the kernel
  nl_cache_free(cache);
  nl_socket_free(sock);

  return;

nla_put_failure:
  throw std::runtime_error("Error constructing nlmsg");
}

void Namespace::create_ns(const std::string &name) {
  std::string netns_path(std::string(NETNS_RUN_DIR) + "/" + name);

  ::mkdir(NETNS_RUN_DIR, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);

  int fd = ::open(netns_path.c_str(), O_RDONLY|O_CREAT|O_EXCL, 0);
  if (fd < 0) {
    throw std::runtime_error(std::string("open error: ") +
                             std::strerror(errno));
  }

  ::close(fd);

  if(::unshare(CLONE_NEWNET) == -1) {
    throw std::runtime_error(std::string("unshare error: ") +
                             std::strerror(errno));
  }

  std::ostringstream os;
  os << "/proc/self/task/" << syscall(__NR_gettid) << "/ns/net";

  if(::mount(os.str().c_str(), netns_path.c_str(), "none", MS_BIND, NULL)) {
    throw std::runtime_error(std::string("mount error: ") +
                             std::strerror(errno));
  }
}

void Namespace::execute_in_worker(std::function<void()> f) {
  Event ev;
  ev.f = f;

  {
    std::lock_guard<std::mutex> lk(mutex);
    queue.push(&ev);
  }

  cv.notify_one();

  std::future<void> barrier_future = ev.barrier.get_future();
  barrier_future.wait();
}


void Namespace::worker() {
  std::unique_lock<std::mutex> lk(mutex);
  while (1) {
    cv.wait(lk, []{return queue.size() >= 1 || finished;});
    if (finished) {
      return;
    }

    Event *ev = queue.front();
    ev->f();
    ev->barrier.set_value();
    queue.pop();
  }
}

void Namespace::stop() {
  {
    std::lock_guard<std::mutex> lk(mutex);
    finished = true;
  }

  cv.notify_all();
  executor->join();
  executor.reset();
}

void Namespace::start() {
  finished = false;

  executor = std::make_unique<std::thread>(worker);
}

} // namespace polycube::polycube
