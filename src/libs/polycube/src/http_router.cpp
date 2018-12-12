/*
 * This file is a reduced version of the original present at
 * https://github.com/oktal/pistache/.
 */

/* router.cc
   Mathieu Stefani, 05 janvier 2016

   Rest routing implementation
*/

#include "polycube/services/http_router.h"
#include <algorithm>

namespace polycube {
namespace service {

namespace Rest {

Request::Request(
        const HttpHandleRequest& request,
        std::vector<TypedParam>&& params,
        std::vector<TypedParam>&& splats)
    : HttpHandleRequest(request)
    , params_(std::move(params))
    , splats_(std::move(splats))
{
}

bool
Request::hasParam(std::string name) const {
    auto it = std::find_if(params_.begin(), params_.end(), [&](const TypedParam& param) {
            return param.name() == name;
    });

    return it != std::end(params_);
}

TypedParam
Request::param(std::string name) const {
    auto it = std::find_if(params_.begin(), params_.end(), [&](const TypedParam& param) {
            return param.name() == name;
    });

    if (it == std::end(params_)) {
        throw std::runtime_error("Unknown parameter");
    }

    return *it;
}

TypedParam
Request::splatAt(size_t index) const {
    if (index >= splats_.size()) {
        throw std::out_of_range("Request splat index out of range");
    }

    return splats_[index];
}

std::vector<TypedParam>
Request::splat() const {
    return splats_;
}

Route::Fragment::Fragment(std::string value)
{
    if (value.empty())
        throw std::runtime_error("Invalid empty fragment");

    init(std::move(value));
}

bool
Route::Fragment::match(const std::string& raw) const {
    if (flags.hasFlag(Flag::Fixed)) {
        return raw == value_;
    }
    else if (flags.hasFlag(Flag::Parameter) || flags.hasFlag(Flag::Splat)) {
        return true;
    }

    return false;
}

bool
Route::Fragment::match(const Fragment& other) const {
    return match(other.value());
}

void
Route::Fragment::init(std::string value) {
    if (value[0] == ':')
        flags.setFlag(Flag::Parameter);
    else if (value[0] == '*') {
        if (value.size() > 1)
            throw std::runtime_error("Invalid splat parameter");
        flags.setFlag(Flag::Splat);
    }
    else {
        flags.setFlag(Flag::Fixed);
    }

    // Let's search for any '?'
    auto pos = value.find('?');
    if (pos != std::string::npos) {
        if (value[0] != ':')
            throw std::runtime_error("Only optional parameters are currently supported");

        if (pos != value.size() - 1)
            throw std::runtime_error("? should be at the end of the string");

        value_ = value.substr(0, pos);
        flags.setFlag(Flag::Optional);
    } else {
        value_ = std::move(value);
    }

    checkInvariant();
}

void
Route::Fragment::checkInvariant() const {
    auto check = [this](std::initializer_list<Flag> exclusiveFlags) {
        for (auto flag: exclusiveFlags) {
            if (!flags.hasFlag(flag)) return;
        }

        throw std::logic_error(
                std::string("Invariant violated: invalid combination of flags for fragment ") + value_);
    };

    check({ Flag::Fixed, Flag::Optional });
    check({ Flag::Fixed, Flag::Parameter });
    check({ Flag::Splat, Flag::Fixed });
    check({ Flag::Splat, Flag::Optional });
    check({ Flag::Splat, Flag::Parameter });
}

std::vector<Route::Fragment>
Route::Fragment::fromUrl(const std::string& url) {
    std::vector<Route::Fragment> fragments;

    std::istringstream iss(url);
    std::string p;

    while (std::getline(iss, p, '/')) {
        if (p.empty()) continue;

        fragments.push_back(Fragment(std::move(p)));
    }

    return fragments;
}

bool
Route::Fragment::isParameter() const {
    return flags.hasFlag(Flag::Parameter);
}

bool
Route::Fragment::isOptional() const {
    return isParameter() && flags.hasFlag(Flag::Optional);
}

bool
Route::Fragment::isSplat() const {
    return flags.hasFlag(Flag::Splat);
}

std::tuple<bool, std::vector<TypedParam>, std::vector<TypedParam>>
Route::match(const HttpHandleRequest& req) const
{
    return match(req.resource());
}

std::tuple<bool, std::vector<TypedParam>, std::vector<TypedParam>>
Route::match(const std::string& req) const
{
#define UNMATCH() std::make_tuple(false, std::vector<TypedParam>(), std::vector<TypedParam>())

    auto reqFragments = Fragment::fromUrl(req);
    if (reqFragments.size() > fragments_.size())
        return UNMATCH();

    std::vector<TypedParam> params;
    std::vector<TypedParam> splats;


    for (std::vector<Fragment>::size_type i = 0; i < fragments_.size(); ++i) {
        const auto& fragment = fragments_[i];
        if (i >= reqFragments.size()) {
            if (fragment.isOptional())
                continue;

            return UNMATCH();
        }

        const auto& reqFragment = reqFragments[i];
        if (!fragment.match(reqFragment))
            return UNMATCH();

        if (fragment.isParameter()) {
            params.push_back(TypedParam(fragment.value(), reqFragment.value()));
        } else if (fragment.isSplat()) {
            splats.push_back(TypedParam(reqFragment.value(), reqFragment.value()));
        }

    }

#undef UNMATCH

    return make_tuple(true, std::move(params), std::move(splats));
}

void
Router::get(std::string resource, Route::Handler handler) {
    addRoute(Http::Method::Get, std::move(resource), std::move(handler));
}

void
Router::post(std::string resource, Route::Handler handler) {
    addRoute(Http::Method::Post, std::move(resource), std::move(handler));
}

void
Router::put(std::string resource, Route::Handler handler) {
    addRoute(Http::Method::Put, std::move(resource), std::move(handler));
}

void
Router::patch(std::string resource, Route::Handler handler) {
    addRoute(Http::Method::Patch, std::move(resource), std::move(handler));
}

void
Router::del(std::string resource, Route::Handler handler) {
    addRoute(Http::Method::Delete, std::move(resource), std::move(handler));
}

void
Router::options(std::string resource, Route::Handler handler) {
    addRoute(Http::Method::Options, std::move(resource), std::move(handler));
}

void
Router::addCustomHandler(Route::Handler handler) {
    customHandlers.push_back(std::move(handler));
}

Router::Status
Router::route(const HttpHandleRequest& req, HttpHandleResponse& response) {
    auto& r = routes[req.method()];
    for (const auto& route: r) {
        bool match;
        std::vector<TypedParam> params;
        std::vector<TypedParam> splats;
        std::tie(match, params, splats) = route.match(req);
        if (match) {
            //route.invokeHandler(Request(req, std::move(params), std::move(splats)), std::move(response));
            route.invokeHandler(Request(req, std::move(params), std::move(splats)), response);
            return Router::Status::Match;
        }
    }

    for (const auto& handler: customHandlers) {
        //auto resp = response.clone();
        //auto result = handler(Request(req, std::vector<TypedParam>(), std::vector<TypedParam>()), std::move(resp));
        auto result = handler(Request(req, std::vector<TypedParam>(), std::vector<TypedParam>()), response);
        if (result == Route::Result::Ok) return Router::Status::Match;
    }

    return Router::Status::NotFound;
}

void
Router::addRoute(Http::Method method, std::string resource, Route::Handler handler)
{
    auto& r = routes[method];
    r.push_back(Route(std::move(resource), method, std::move(handler)));
}


namespace Routes {

void Get(Router& router, std::string resource, Route::Handler handler) {
    router.get(std::move(resource), std::move(handler));
}

void Post(Router& router, std::string resource, Route::Handler handler) {
    router.post(std::move(resource), std::move(handler));
}

void Put(Router& router, std::string resource, Route::Handler handler) {
    router.put(std::move(resource), std::move(handler));
}

void Patch(Router& router, std::string resource, Route::Handler handler) {
    router.patch(std::move(resource), std::move(handler));
}

void Delete(Router& router, std::string resource, Route::Handler handler) {
    router.del(std::move(resource), std::move(handler));
}

void Options(Router& router, std::string resource, Route::Handler handler) {
    router.options(std::move(resource), std::move(handler));
}

} // namespace Routes

} // namespace Rest

}  // namespace service
}  // namespace polycube
