/*
 * Copyright 2021 Leonardo Di Giovanna
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


#include "SrcIpRewrite.h"
#include "K8slbrp.h"

using namespace polycube::service;

enum {
    FROM_FRONTEND = 0,
    FROM_BACKEND = 1,
};

SrcIpRewrite::SrcIpRewrite(K8slbrp &parent, const SrcIpRewriteJsonObject &conf)
    : SrcIpRewriteBase(parent) {
    logger()->info("[Constructor] Creating SrcIpRewrite instance");

    if (conf.newIpRangeIsSet()) {
        setNewIpRange(conf.getNewIpRange());
    }

    if (conf.ipRangeIsSet()) {
        setIpRange(conf.getIpRange());
    }
}

SrcIpRewrite::~SrcIpRewrite() {}

void SrcIpRewrite::update(const SrcIpRewriteJsonObject &conf) {
    if (!conf.ipRangeIsSet() || !conf.newIpRangeIsSet())
        return;

    setIpRange(conf.getIpRange());
    setNewIpRange(conf.getNewIpRange());

    std::string old_ip_net = ip_range.substr(0, ip_range.find('/'));
    std::string old_mask_len =
            ip_range.substr(ip_range.find('/') + 1, std::string::npos);
    std::string new_ip_net = new_ip_range.substr(0, new_ip_range.find('/'));
    std::string new_mask_len =
            new_ip_range.substr(new_ip_range.find('/') + 1, std::string::npos);

    // TODO: change datamodel to fix it
    if (old_mask_len != new_mask_len) {
        throw std::runtime_error("both masks have to be the same");
    }

    uint32_t old_ip_dec;
    uint32_t new_ip_dec;

    uint32_t new_mask_dec = 0xFFFFFFFF;
    uint32_t old_mask_dec = 0xFFFFFFFF;
    new_mask_dec <<= 32 - std::stoi(new_mask_len);
    old_mask_dec <<= 32 - std::stoi(old_mask_len);
    new_mask_dec = ~new_mask_dec;
    old_mask_dec = ~old_mask_dec;

    auto src_ip_rewrite =
            parent_.get_hash_table<src_ip_r_key, src_ip_r_value>("src_ip_rewrite");

    src_ip_r_key key{
            .netmask_len = uint32_t(std::stoi(old_mask_len)),
            .network = utils::ip_string_to_nbo_uint(old_ip_net),
    };

    src_ip_r_value value{
            .sense = FROM_BACKEND,
            .net = utils::ip_string_to_nbo_uint(new_ip_net),
            .mask = new_mask_dec,
    };

    src_ip_rewrite.set(key, value);

    // create entry for packets comming from the service
    src_ip_r_key key0{
            .netmask_len = uint32_t(std::stoi(new_mask_len)),
            .network = utils::ip_string_to_nbo_uint(new_ip_net),
    };

    src_ip_r_value value0{
            .sense = FROM_BACKEND,  // TODO: is this value right?
            .net = utils::ip_string_to_nbo_uint(old_ip_net),
            .mask = old_mask_dec,
    };

    src_ip_rewrite.set(key0, value0);

    // update variables in the object
    mask = new_mask_dec;
    net = new_ip_dec;
}

std::string SrcIpRewrite::getIpRange() {
    return ip_range;
}

void SrcIpRewrite::setIpRange(const std::string &value) {
    ip_range = value;
}

std::string SrcIpRewrite::getNewIpRange() {
    return new_ip_range;
}

void SrcIpRewrite::setNewIpRange(const std::string &value) {
    new_ip_range = value;
}


