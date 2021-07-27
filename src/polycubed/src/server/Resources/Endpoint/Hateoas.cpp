/*
 * Copyright 2020 The Polycube Authors
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


#include "Hateoas.h"
#include "Service.h"
#include "ParentResource.h"

/// Hateoas record that will be used to support the standard
auto record = R"({
    "rel": "",
    "href": ""
    })"_json;


std::unordered_map<std::string, std::string> Hateoas::root_routes;


void Hateoas::HateoasSupport(const Request &request,
        std::vector<Response> &errors,
        const std::string &host, const std::string &port,
        const std::vector<std::shared_ptr<polycube::polycubed::Rest
        ::Resources::Body::Resource>> &children) {
    // building "self" endpoint
    std::string self_endpoint(request.resource());
    char delim = '/';
    if (self_endpoint.back() != delim)
        self_endpoint += delim;
    auto hateoas_record = record;
    std::string host_and_port("//" + host + ":" + port);
    auto js_hateoas = Hateoas::writeSelfRecord(host_and_port, self_endpoint);

    /* here all children resources are processed in
     * order to extract their endpoints */
    try {
        for (auto child : children) {
            auto child_resource_ptr = std::dynamic_pointer_cast<polycube::polycubed
            ::Rest::Resources::Endpoint::Resource>(child);
            if (child_resource_ptr == nullptr)
                continue;

            // building the final endpoint to the next level n+1
            auto final_href = host_and_port + self_endpoint + child->Name() + delim;
            hateoas_record["href"] = final_href;
            hateoas_record["rel"] = child->Name();
            js_hateoas.push_back(hateoas_record);
        }

        // then attaching link to support hateoas
        Hateoas::attach_links(errors, js_hateoas);
    } catch (...) {
        spdlog::get("polycubed")->error(
                "Hateoas - HateoasSupport error!");
    }
}


void Hateoas::HateoasSupport_multiple(const Request &request,
        std::vector<Response> &errors,
        const std::string &host, const std::string &port,
        const polycube::polycubed::Rest::Resources::Endpoint::Resource *lr) {

    if (lr == nullptr) {
        spdlog::get("polycubed")->error(
                "Hateoas - HateoasSupport_multiple - "
                "Resource lr seems be null! "
                "That's not possible for a ListResource! "
                "Skipping _links support.");
        return;
    }

    // building "self" endpoint
    char delim = '/';
    std::string self_endpoint(request.resource());
    if (self_endpoint.back() != delim)
        self_endpoint += delim;
    size_t level_self = std::count(self_endpoint.begin(),
            self_endpoint.end(), delim);

    std::string lr_endpoint(lr->Endpoint());
    if (lr_endpoint.back() != delim)
        lr_endpoint += delim;
    size_t level_lr = std::count(lr_endpoint.begin(),
            lr_endpoint.end(), delim);

    // writing "self" record
    std::string host_and_port("//" + host + ":" + port);
    auto js_hateoas = Hateoas::writeSelfRecord(host_and_port, self_endpoint);
    std::string base(host_and_port + self_endpoint);

    if (level_self >= level_lr) {
        spdlog::get("polycubed")->error(
                "Hateoas - HateoasSupport_multiple - "
                "level_self [{0}] >= level_lr [{1}]! "
                "Skipping _links support.", level_self, level_lr);
        return;
    }

    // getting the key to resolve the dynamic endpoint
    auto key = Hateoas::getToken_level(lr_endpoint, level_self - 1);
    if (!Hateoas::is_key(key)) {
        spdlog::get("polycubed")->error(
                "Hateoas - HateoasSupport_multiple - no key found! "
                "Skipping _links support.");
        return;
    }
    key.erase(key.begin());

    /* using the key to resolve the value used to build the final endpoint
     * js_hateoas json is populated using next level endpoints.
     * writeNextlevel will resolve the values and build all hateoas entries */
    try {
        if (!Hateoas::writeNextlevel(errors, js_hateoas, base, key)) {
            std::string type(Hateoas::getLastToken(request));

            if (key.find("_name") != std::string::npos)
                key = "name";
            if (!Hateoas::writeNextlevel(errors, js_hateoas, base, key)) {
                spdlog::get("polycubed")->error(
                        "Hateoas - HateoasSupport_multiple - "
                        "no parameter to make the resolution found! "
                        "Skipping _links support.");
                return;
            }
        }

        // then attaching _links to the response
        Hateoas::attach_links(errors, js_hateoas);
    } catch (...) {
        spdlog::get("polycubed")->error(
                "Hateoas - HateoasSupport_multiple error!");
    }
}


void Hateoas::HateoasSupport_services(const Request &request,
        std::vector<Response> &response,
        const std::unordered_set<std::string> &cube_names,
        const std::string &host, const std::string &port) {

    // building "self" endpoint
    std::string self_endpoint(request.resource());
    char delim = '/';
    if (self_endpoint.back() != delim)
        self_endpoint += delim;
    auto hateoas_record = record;
    std::string host_and_port("//" + host + ":" + port);

    // writing "self" record to the json
    auto js_hateoas = Hateoas::writeSelfRecord(host_and_port, self_endpoint);

    // for each cube an Hateoas record is attached to the response
    try {
        std::for_each(cube_names.begin(), cube_names.end(), [&](auto &cube_name){
            auto final_href = host_and_port + self_endpoint + cube_name + delim;
            hateoas_record["rel"] = cube_name;
            hateoas_record["href"] = final_href;
            js_hateoas.push_back(hateoas_record);
        });

        // then _links are ben attached to the response
        Hateoas::attach_links(response, js_hateoas);
    } catch (...) {
        spdlog::get("polycubed")->error(
                "Hateoas - HateoasSupport_services error!");
    }
}


json Hateoas::HateoasSupport_root(const Pistache::Rest::Request &request,
        const std::string &host, const std::string &port) {
    // building "self" endpoint
    std::string self_endpoint(request.resource());
    char delim = '/';
    if (self_endpoint.back() != delim)
        self_endpoint += delim;
    auto hateoas_record = record;
    std::string host_and_port("//" + host + ":" + port);
    // writing "self" record to the json
    auto js_hateoas = Hateoas::writeSelfRecord(host_and_port, self_endpoint);

    /* for each route just after the root an Hateoas record
     * is attached to the json (js_hateoas) */
    try {
        std::for_each(Hateoas::root_routes.begin(), Hateoas::root_routes.end(),
                      [&](auto &route_pair){
                          auto final_href = host_and_port + route_pair.second + delim;
                          hateoas_record["rel"] = route_pair.first;
                          hateoas_record["href"] = final_href;
                          js_hateoas.push_back(hateoas_record);
                      }
        );

        return js_hateoas;
    } catch (...) {
        spdlog::get("polycubed")->error(
                "Hateoas - HateoasSupport_root error!");
    }
}


json Hateoas::writeSelfRecord(std::string &host_and_port,
        std::string &self_endpoint) {
    // here the self record is attached to the response json
    auto js_hateoas = json::array();
    auto hateoas_self_record = record;
    hateoas_self_record["rel"] = "self";
    hateoas_self_record["href"] = host_and_port + self_endpoint;
    js_hateoas.push_back(hateoas_self_record);
    return js_hateoas;
}


void Hateoas::attach_links(std::vector<Response> &errors, json &js_hateoas){
    // here the json containing the Hateoas result is attached to the response
    json js;
    if (errors[0].message != nullptr)
        js = json::parse(errors[0].message);
    js.push_back({"_links", js_hateoas});
    std::string json_dump(std::move(js.dump()));
    if (errors[0].message != nullptr)
        ::free(errors[0].message);
    errors[0].message = new char[json_dump.size() + 1];
    std::copy(json_dump.begin(), json_dump.end(), errors[0].message);
    errors[0].message[json_dump.size()] = '\0';
}


bool Hateoas::writeNextlevel(std::vector<Response> &errors, json &js_hateoas,
                             std::string &base, std::string &key) {
    // using the key, values are extracted from the response json
    auto response_json = json::parse(errors[0].message);
    bool found = false;

    // finding values in the response
    for (auto& it : response_json.items()) {
        if (it.value().find(key) != it.value().end()) {
            found = true;
            auto hateoas_record = record;
            std::stringstream ss;
            ss << base;
            // endpoints are written in the json of the response
            if (it.value().find(key)->is_string()) {
                ss << it.value().find(key)->get<std::string>();
            } else {
                ss << *it.value().find(key);
            }
            ss << "/";
            hateoas_record["rel"] = *it.value().find(key);
            hateoas_record["href"] = ss.str();
            // every record is pushed back to the json
            js_hateoas.push_back(hateoas_record);
        }
    }
    return found;
}


std::string Hateoas::getToken_level(const std::string &endpoint,
        const size_t level) {
    return Hateoas::split_to_tokens(endpoint).at(level);
}


bool Hateoas::is_key(std::string &token) {
    // if a token is a key must be ":<key>"
    return token.front() == ':';
}


std::vector<std::string> Hateoas::split_to_tokens(const std::string &href) {
    // here a single endpoint is splitted to tokens
    std::string delim("/");
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;
    do
    {
        // finding the delimiter, splitting, then pushing back the token in the vector
        pos = href.find(delim, prev);
        if (pos == std::string::npos) pos = href.length();
        std::string token = href.substr(prev, pos-prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    }
    while (pos < href.length() && prev < href.length());
    return tokens;
}


std::string Hateoas::getLastToken(const Request &request) {
    // return the last token of the resource
    std::vector<std::string> tokens =
            Hateoas::split_to_tokens(request.resource());
    return tokens.back();
}


void Hateoas::addRoute(const std::string &&route_name,
        const std::string &base) {
    Hateoas::root_routes.insert({route_name, base + route_name});
}


void Hateoas::removeRoute(const std::string &service_name) {
    Hateoas::root_routes.erase(service_name);
}
