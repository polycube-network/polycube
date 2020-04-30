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


#ifndef POLYCUBE_HATEOAS_H
#define POLYCUBE_HATEOAS_H

#include <spdlog/spdlog.h>

#include "Resource.h"
#include "../Body/Resource.h"
#include "polycube/services/json.hpp"

using json = nlohmann::json;
using Pistache::Rest::Request;
using Pistache::Http::ResponseWriter;


/**
 *              ***Hateoas support for Polycube***
 *
 * The goal is provide a method to the client to know the endpoints
 * offered to communicate with the server.
 * Next level links must be attached to the server response.
 *
 * Server responses are intercepted (see Endpoint/ParentResource.cpp
 * and Endpoint/ListResource.cpp) and information to support the
 * hateoas standard is added to them.
 * In particular, dynamic endpoints that can be used to request
 * resource information are resolved explicitly in order to provide
 * only valid endpoints that can be used by the client.
 *
 * In fact in Polycube endpoints can have dynamic routes (in particular
 * those of ListResource), in this case hateoas provides the
 * resolution of these providing the client with valid endpoints
 * that it can use to explore the service.
 *          http://pistache.io/guide/#dynamic-routes
 *
 * In case of endpoints with dynamic route
 * (e.g. /polycube/v1/simplebridge/:name/fdb/:entry/) the dynamic
 * token (:entry) is resolved (see Hateoas::HateoasSupport_multiple
 * and Hateoas::writeNextlevel) using the "entry" key to search the
 * associated value that can be used to build the final endpoint.
 *
 *    e.g.
 *
 *    from    ->  //polycube/v1/simplebridge/:name/fdb/:entry/
 *    to      ->  //polycube/v1/simplebridge/br1/fdb/to_veth1/
 *
 *
 * A possible output for GET -> localhost:9000/polycube/v1/simplebridge/sb1/ :
 *
 * "_links": [
 *      {
 *          "rel": "self",
 *          "href": "//127.0.0.1:9000/polycube/v1/simplebridge/sb1/"
 *      },
 *      {
 *          "rel": "uuid",
 *          "href": "//127.0.0.1:9000/polycube/v1/simplebridge/sb1/uuid/"
 *      },
 *      {
 *          "rel": "type",
 *          "href": "//127.0.0.1:9000/polycube/v1/simplebridge/sb1/type/"
 *      },
 *      {
 *          "rel": "service-name",
 *          "href": "//127.0.0.1:9000/polycube/v1/simplebridge/sb1/service-name/"
 *      },
 *      ...
 */
class Hateoas {

private:

    Hateoas(){}

    /**
     * write the self hateoas record.
     *
     * @param[in] host_and_port host and port of the server (i.e. //127.0.0.1:9000/)
     * @param[in] self_endpoint self endpoint to attach at host and port
     * @returns                 the json containing the entry of the self endpoint
     */
    static json writeSelfRecord(std::string &host_and_port,
                                std::string &self_endpoint);

    /**
     * attach Hateoas links to the errors that will be sent to the client.
     *
     * @param[out] errors       the response that will be sent to the client
     * @param[in] js_hateoas    links to attach to the errors
     */
    static void attach_links(std::vector<Response> &errors, json &js_hateoas);

    /**
     * get the last token of the request.
     *
     * @param[in] request       the request provided to the client
     * @returns                 the last token of the request
     */
    static std::string getLastToken(const Request &request);

    /**
     * using a key, that member function will search the right value in the
     * errors that are provided from the cube in order to resolve
     * dynamic endpoints.
     *
     * @param[in] errors        the response that will be sent to the client
     * @param[out] js_hateoas   the json where Hateoas records will be stored
     * @param[in] base          current level endpoint
     * @param[in] key           the key that will be used to find next level values
     *                          that will make up final endpoints
     * @returns                 true if the key was found, false otherwise
     */
    static bool writeNextlevel(std::vector<Response> &errors, json &js_hateoas,
                               std::string& base, std::string &key);

    /**
     * provides the token of the specified level for a given endpoint
     *
     * @param[in] endpoint      the endpoint to process
     * @param[in] level         the level required
     * @returns                 the token of the required level for a given endpoint
     */
    static std::string getToken_level(const std::string &endpoint,
            const size_t level);

    /**
     * check if the token provided is a key or not
     *
     * @param[in] token         the token to process
     * @returns                 true if the token is a key, false otherwise
     */
    static bool is_key(std::string &token);

    /**
     * split the href provided to tokens
     *
     * @param[in] href          the href to split
     * @returns                 tokens
     */
    static std::vector<std::string> split_to_tokens(const std::string &href);

    /// map containing the routes only of the level just after the root
    static std::unordered_map<std::string, std::string> root_routes;


public:

    Hateoas(Hateoas const&)         = delete;
    void operator=(Hateoas const&)  = delete;

    /**
     * Hateoas support for a ParentResource.
     * Children's links are attached to the response.
     *
     * @param[in] request       the request of the client
     * @param[in,out] errors    the response to which the links will be added
     * @param[in] host          server host address
     * @param[in] port          server port
     * @param[in] children      children of the processed ParentResource node
     */
    static void HateoasSupport(const Request &request,
            std::vector<Response> &errors,
            const std::string &host, const std::string &port,
            const std::vector<std::shared_ptr<polycube::polycubed
            ::Rest::Resources::Body::Resource>> &children);

    /**
     * Hateoas support for a ListResource.
     * Dynamic endpoint routes are resolved by processing
     * the response json of the cube list resource.
     *
     * @param[in] request       the request of the client
     * @param[in,out] errors    the response to which the links will be added
     * @param[in] host          server host address
     * @param[in] port          server port
     * @param[in] lr            pointer to the ListResource that are processed
     */
    static void HateoasSupport_multiple(const Request &request,
            std::vector<Response> &errors,
            const std::string &host, const std::string &port,
            const polycube::polycubed
            ::Rest::Resources::Endpoint::Resource *lr);

    /**
     * Hateoas support for all cubes (Services) of the same type.
     * e.g. simplebridge, router, nat etc...
     *
     * @param[in] request       the request of the client
     * @param[in,out] response  the response to which the links will be added
     * @param[in] cube_names    running cubes name to the same type
     * @param[in] host          server host address
     * @param[in] port          server port
     */
    static void HateoasSupport_services(const Request &request,
            std::vector<Response> &response,
            const std::unordered_set<std::string> &cube_names,
            const std::string &host, const std::string &port);

    /**
     * Hateoas support for the root.
     *
     * @param[in] request       the request of the client
     * @param[in] host          server host address
     * @param[in] port          server port
     * @returns                 json that contains the response that will
     *                          be sent to the client
     */
    static json HateoasSupport_root(const Pistache::Rest::Request &request,
            const std::string &host, const std::string &port);

    /**
     * Add a route from the root.
     * That kind of routes must be stored in order to provide them.
     *
     * @param[in] route_name    the name of the route
     * @param[in] base          the base endpoint of the root
     */
    static void addRoute(const std::string &&route_name,
            const std::string &base);

    /**
     * Remove a route from the root for a specific service.
     *
     * @param[in] service_name  the name of the service
     *                          (e.g. simplebridge, router..)
     */
    static void removeRoute(const std::string &service_name);
};


#endif //POLYCUBE_HATEOAS_H
