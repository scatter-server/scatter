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
#include <boost/asio.hpp>
#include <toolboxpp.h>

namespace wss {
namespace client {

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

    QueryBuilder(std::string url, bool https, Method method) :
        https(https),
        host(std::move(url)),
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

    void buildOStream(std::ostream *os) {
        using toolboxpp::strings::glue;
        const bool isPost = method == POST || method == PUT;
        std::vector<std::string> queryForGlue;
        std::stringstream qss;

        for (auto q: query) {
            qss << q.first << "=" << q.second;
            queryForGlue.push_back(qss.str());
            qss.str("");
            qss.clear();
        }

        if (!queryForGlue.empty() && !isPost) {
            *os << methodToString(method) << " " << path << "?" << glue("&", queryForGlue) << " HTTP/1.1\r\n";
        } else {
            *os << methodToString(method) << " " << path << " HTTP/1.1\r\n";
        }

        addHeader("Host", host);
        addHeader("Connection", "close");
        addHeader("Accept", "*/*");
        addHeader("User-Agent", "WsServer/1.0 HttpClient");
        addHeader("Upgrade-Insecure-Requests", "1");

        std::string postData;
        if (isPost) {
            postData = glue("&", queryForGlue);
            addHeader("Content-Type", "application/x-www-form-urlencoded");
            addHeader("Content-Length", postData.length());
        }

        for (auto &s: headers) {
            *os << s.first << ": " << s.second << "\r\n";
        }

        *os << "\r\n";

        if (isPost) {
            *os << postData;
        }
    }
};

}
}

#endif //WSSERVER_STANDALONE_QUERYBUILDER_HPP
