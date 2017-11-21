/**
 * wsserver_standalone
 * QueryBuilder.hpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_STANDALONE_QUERYBUILDER_HPP
#define WSSERVER_STANDALONE_QUERYBUILDER_HPP

#include <unordered_map>
#include <string>
#include <utility>
#include <iostream>
#include <exception>
#include <regex>
#include <boost/asio.hpp>
#include <toolboxpp.h>
#include <curl/curl.h>

namespace wss {
namespace web {

using QueryData = std::vector<std::pair<std::string, std::string>>;
using HeaderData = std::unordered_map<std::string, std::string>;

class QueryBuilder {
 public:
    enum Method {
      GET, POST, PUT, DELETE
    };
 private:
    std::string host, path;
    Method method;
    bool https;
    QueryData query;
    HeaderData headers;

    Method methodFromString(const std::string &m) const {
        Method out;
        using toolboxpp::strings::equalsIgnoreCase;

        if (equalsIgnoreCase(m, "GET")) {
            out = GET;
        } else if (equalsIgnoreCase(m, "POST")) {
            out = POST;
        } else if (equalsIgnoreCase(m, "PUT")) {
            out = PUT;
        } else if (equalsIgnoreCase(m, "DELETE")) {
            out = DELETE;
        } else {
            out = GET;
        }

        return out;
    }

    std::string methodToString(Method m) const {
        std::string out;
        switch (m) {
            case GET: out = "GET";
                break;
            case POST: out = "POST";
                break;
            case PUT: out = "PUT";
                break;
            case DELETE: out = "DELETE";
                break;
            default: out = "GET";
        }

        return out;
    }

 public:
    static QueryBuilder parse(const std::string &toParse) {
        const std::string pattern = R"((http|https)\:\/\/([a-zA-Z0-9\.\-]+)(.*))";
        std::smatch match = toolboxpp::strings::matchRegexp(pattern, toParse);
        if (match.size() < 4) {
            throw std::runtime_error("Unable to parse url: " + toParse);
        }

        QueryBuilder builder(match[2], match[3], match[1] == "https");

        return builder;
    }

    QueryBuilder() { };

    explicit QueryBuilder(const std::string &host) :
        https(false),
        host(host),
        path("/"),
        method(GET) {
    }

    QueryBuilder(const std::string &host, const std::string &path, bool https) :
        https(https),
        host(host),
        path(path),
        method(GET) {
    }

    QueryBuilder(std::string host, bool https, Method method) :
        https(https),
        host(std::move(host)),
        method(method) {
    }

    QueryBuilder(QueryBuilder &&another) = default;
    QueryBuilder(const QueryBuilder &another) = default;
    ~QueryBuilder() {
        query.clear();
    }

    QueryBuilder &addParam(const std::string &key, const char *value) {
        query.emplace_back(key, std::string(value));
        return *this;
    }

    QueryBuilder &addParam(const std::string &key, const std::string &value) {
        query.emplace_back(key, value);
        return *this;
    }

    QueryBuilder &addParam(const std::string &key, double value) {
        return addParam(key, std::to_string(value));
    }

    QueryBuilder &addParam(const std::string &key, int value) {
        return addParam(key, std::to_string(value));
    }

    QueryBuilder &addParam(const std::string &key, long value) {
        return addParam(key, std::to_string(value));
    }

    QueryBuilder &addParam(const std::string &key, unsigned int value) {
        return addParam(key, std::to_string(value));
    }

    QueryBuilder &addParam(const std::string &key, unsigned long value) {
        return addParam(key, std::to_string(value));
    }

    QueryBuilder &addParam(const std::string &key, bool value) {
        query.emplace_back(key, value ? "1" : "0");
        return *this;
    }

    QueryBuilder &addHeader(const std::string &key, const char *value) {
        headers[key] = std::string(value);
        return *this;
    }

    QueryBuilder &addHeader(const std::string &key, const std::string &value) {
        headers[key] = value;
        return *this;
    }

    QueryBuilder &addHeader(const std::string &key, double value) {
        return addHeader(key, std::to_string(value));
    }

    QueryBuilder &addHeader(const std::string &key, int value) {
        return addHeader(key, std::to_string(value));
    }

    QueryBuilder &addHeader(const std::string &key, long value) {
        return addHeader(key, std::to_string(value));
    }

    QueryBuilder &addHeader(const std::string &key, unsigned int value) {
        return addHeader(key, std::to_string(value));
    }

    QueryBuilder &addHeader(const std::string &key, unsigned long value) {
        return addHeader(key, std::to_string(value));
    }

    QueryBuilder &addHeader(const std::string &key, bool value) {
        return addHeader(key, value ? "1" : "0");
    }

    QueryBuilder &removeHeader(const std::string &key) {
        if (!headers.count(key)) return *this;

        headers.erase(key);
        return *this;
    }

    QueryBuilder &setMethod(Method m) {
        method = m;
        return *this;
    }

    QueryBuilder &setMethod(const std::string &m) {
        return setMethod(methodFromString(m));
    }

    QueryBuilder &setHttps(bool https) {
        this->https = https;
        return *this;
    }

    QueryBuilder &setHost(std::string h) {
        host = std::move(h);
        return *this;
    }

    QueryBuilder &setPath(std::string path) {
        path = std::move(path);
        return *this;
    }

    std::string getHost() const {
        return host;
    }

    std::string getProtocol() const {
        return https ? "https" : "http";
    }

    std::string getPath() const {
        return path;
    }

    bool hasHeaders() const {
        return !headers.empty();
    }

    std::string getUrl() const {
        std::stringstream ss;
        ss << (https ? "https" : "http");
        ss << "://" << host << "/" << path;

        return ss.str();
    }

    std::vector<std::string> getHeadersGlued() const {
        std::vector<std::string> out;
        for (auto &h: headers) {
            out.push_back(h.first + ": " + h.second);
        }

        return out;
    }
};

}
}

#endif //WSSERVER_STANDALONE_QUERYBUILDER_HPP
