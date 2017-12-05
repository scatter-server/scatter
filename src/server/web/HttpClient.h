/**
 * wsserver_standalone
 * HttpClient.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_STANDALONE_HTTPCLIENT_H
#define WSSERVER_STANDALONE_HTTPCLIENT_H

#include <string>
#include <iostream>
#include <istream>
#include <ostream>
#include <boost/algorithm/string/trim.hpp>
#include <toolboxpp.h>
#include <curl/curl.h>
#include <server_http.hpp>
#include "../defs.h"

namespace wss {
namespace web {

typedef std::pair<std::string, std::string> KeyValue;
typedef std::vector<KeyValue> KeyValueVector;

// avoiding cross-references
using WebHttpStatus = SimpleWeb::StatusCode;
using WebHttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using WebHttpResponse = std::shared_ptr<WebHttpServer::Response>;
using WebHttpRequest = std::shared_ptr<WebHttpServer::Request>;

class IOContainer {
 protected:
    KeyValueVector headers;
    std::string body;

 public:
    IOContainer() :
        body() { }

    void setBody(const std::string &body) {
        this->body = body;
        setHeader({"Content-Length", std::to_string(this->body.length())});
    }

    void setBody(std::string &&body) {
        this->body = std::move(body);
        setHeader({"Content-Length", std::to_string(this->body.length())});
    }

    void setHeader(KeyValue &&keyValue) {
        using toolboxpp::strings::equalsIgnoreCase;
        bool found = false;
        for (auto &kv: headers) {
            if (equalsIgnoreCase(kv.first, keyValue.first)) {
                kv.second = keyValue.second;
                found = true;
            }
        }

        if (!found) {
            return addHeader(std::move(keyValue));
        }
    }

    bool hasHeader(const std::string &name) const {
        using toolboxpp::strings::equalsIgnoreCase;
        for (auto &h: headers) {
            if (equalsIgnoreCase(h.first, name)) {
                return true;
            }
        }

        return false;
    }

    std::pair<std::string, std::string> getHeaderPair(const std::string &headerName) const {
        using toolboxpp::strings::equalsIgnoreCase;
        for (auto &h: headers) {
            if (equalsIgnoreCase(h.first, headerName)) {
                return {h.first, h.second};
            }
        }

        return {};
    }

    std::string getHeader(const std::string &headerName) const {
        using toolboxpp::strings::equalsIgnoreCase;
        for (auto &h: headers) {
            if (equalsIgnoreCase(h.first, headerName)) {
                return h.second;
            }
        }

        return std::string();
    }

    bool compareHeaderValue(const std::string &headerName, const std::string &comparable) const {
        if (!hasHeader(headerName)) return false;
        return getHeader(headerName) == comparable;
    }

    void addHeader(const std::string &key, const std::string &value) {
        headers.emplace_back(key, value);
    }

    void addHeader(const KeyValue &keyValue) {
        headers.push_back(std::move(keyValue));
    }

    void addHeader(KeyValue &&keyValue) {
        headers.push_back(std::move(keyValue));
    }

    std::string getBody() const {
        return body;
    }

    const char *getBodyC() const {
        const char *out = body.c_str();
        return out;
    }

    bool hasBody() const {
        return !body.empty();
    }

    bool hasHeaders() const {
        return !headers.empty();
    }

    KeyValueVector getHeaders() const {
        return headers;
    }

    std::vector<std::string> getHeadersGlued() const {
        std::vector<std::string> out;
        for (auto &h: headers) {
            out.emplace_back(h.first + ": " + h.second);
        }

        return out;
    }

};

class Request : public IOContainer {
 public:
    enum Method {
      GET, POST, PUT, DELETE
    };
 private:
    KeyValueVector params;
    std::string url;
    Method method;

 public:
    Request() : IOContainer(),
                url(),
                method(GET) { }
    explicit Request(const std::string &url) : IOContainer(),
                                               url(url),
                                               method(GET) { }

    explicit Request(std::string &&url) : IOContainer(),
                                          url(std::move(url)),
                                          method(GET) { }

    explicit Request(const WebHttpRequest &sRequest) : IOContainer(),
                                                       url(sRequest->path),
                                                       method(methodFromString(sRequest->method)) {
        for (auto &h: sRequest->header) {
            addHeader(h.first, h.second);
        }

        std::string query = sRequest->query_string;
        if (query[0] == '?') {
            query = query.substr(1, query.length());
        }

        std::vector<std::string> pairs = toolboxpp::strings::split(query, "&");

        for (const auto &param: pairs) {
            addParam(toolboxpp::strings::splitPair(param, "="));
        }
    }

    Method methodFromString(const std::string &methodName) const {
        using toolboxpp::strings::equalsIgnoreCase;

        if (equalsIgnoreCase(methodName, "POST")) {
            return Method::POST;
        } else if (equalsIgnoreCase(methodName, "PUT")) {
            return Method::PUT;
        } else if (equalsIgnoreCase(methodName, "DELETE")) {
            return Method::DELETE;
        }

        return Method::GET;
    }

    std::string methodToString(Method methodName) const {
        std::string out;

        switch (methodName) {
            case Method::POST:out = "POST";
                break;
            case Method::PUT:out = "PUT";
                break;
            case Method::DELETE:out = "DELETE";
                break;

            default:out = "GET";
                break;
        }

        return out;
    }

    void setUrl(const std::string &url) {
        this->url = url;
    }

    void setUrl(std::string &&url) {
        this->url = std::move(url);
    }

    void setMethod(Method method) {
        this->method = method;
    }

    void addParam(KeyValue &&keyValue) {
        params.push_back(std::move(keyValue));
    }

    std::string getUrl() const {
        return url;
    }

    Method getMethod() const {
        return method;
    }

    bool hasParams() const {
        return !params.empty();
    }

    std::string getUrlWithParams() const {
        std::string combined = url;
        if (!params.empty()) {
            std::stringstream ss;
            std::vector<std::string> enc;
            for (auto &p: params) {
                ss << p.first << "=" << p.second;
                enc.push_back(ss.str());
                ss.str("");
                ss.clear();
            }
            std::string glued = toolboxpp::strings::glue("&", enc);

            if (toolboxpp::strings::hasSubstring(url, "?")) {
                combined += "&" + glued;
            } else {
                combined += "?" + glued;
            }
        }

        return combined;
    }

    KeyValueVector getParams() const {
        return params;
    }
};

class Response : public IOContainer {
 public:
    int status = 200;
    std::string statusMessage;
    std::string data;
    std::string _headersBuffer;

    nlohmann::json parseJson() {
        if (data.empty()) {
            return nlohmann::json();
        }

        nlohmann::json out;
        try {
            out = nlohmann::json::parse(data);
        } catch (const std::exception &e) {
            std::cerr << "Can't parse incoming json data: " << e.what() << std::endl;
        }

        return out;
    }

    KeyValueVector parseFormUrlEncode() const {
        if (data.empty()) {
            return {};
        }

        std::vector<std::string> groups = toolboxpp::strings::split(data, '&');

        KeyValueVector kvData;
        for (auto &&s: groups) {
            kvData.push_back(toolboxpp::strings::splitPair(s, "="));
        }

        return kvData;
    };

    void dump() const {
        std::cout << "Response: " << std::endl
                  << "  Status: " << status << std::endl
                  << " Message: " << statusMessage << std::endl
                  << "    Body: " << data << std::endl
                  << " Headers:\n";
        for (auto h: headers) {
            std::cout << "\t" << h.first << ": " << h.second << std::endl;
        }
    }

    bool isSuccess() const {
        return status >= 200 && status < 400;
    }
};

class HttpClient {
 private:
    bool verbose = false;
    long connectionTimeout = 10L;

    static size_t handleResponseData(void *buffer, size_t size, size_t nitems, void *userData) {
        ((Response *) userData)->data.append((char *) buffer, size * nitems);
        return size * nitems;
    }

    static size_t handleResponseHeaders(void *buffer, size_t size, size_t nitems, void *userData) {
        ((Response *) userData)->_headersBuffer.append((char *) buffer, size * nitems);
        return size * nitems;
    }

 public:
    HttpClient() {
        curl_global_init(CURL_GLOBAL_ALL);
    }

    ~HttpClient() {
        curl_global_cleanup();
    }

    void enableVerbose(bool enable) {
        verbose = enable;
    }

    Response execute(Request &request) {
        CURL *curl;
        CURLcode res = CURLE_OK;

        curl = curl_easy_init();
        Response resp;
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, request.getUrlWithParams().c_str());

            bool isPost = false;
            switch (request.getMethod()) {
                case Request::Method::POST:curl_easy_setopt(curl, CURLOPT_POST, 1L);
                    isPost = true;
                    break;
                case Request::Method::PUT:curl_easy_setopt(curl, CURLOPT_PUT, 1L);
                    isPost = true;
                    break;
                case Request::Method::DELETE:curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
                    isPost = false;
                    break;
                default:break;
            }

            if (isPost && request.hasBody()) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.getBodyC());
            }

            curl_easy_setopt(curl, CURLOPT_TIMEOUT, connectionTimeout);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &HttpClient::handleResponseData);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &HttpClient::handleResponseHeaders);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &resp);

            if (request.hasHeaders()) {
                struct curl_slist *headers = nullptr;
                for (auto &h: request.getHeadersGlued()) {
                    L_DEBUG_F("Request", "Header -> %s", h.c_str());
                    headers = curl_slist_append(headers, h.c_str());
                }

                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            }

            if (verbose) {
                curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            }

            res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                resp.status = -1;
                resp.statusMessage = "CURL error: " + std::string(curl_easy_strerror(res));
            } else {
                std::vector<std::string> headerLines = toolboxpp::strings::split(resp._headersBuffer, "\r\n");
                for (auto &header: headerLines) {
                    if (toolboxpp::strings::hasSubstring(header, "HTTP")) {
                        std::smatch match = toolboxpp::strings::matchRegexp(R"(HTTP\/\d\.\d.(\d+).(.*))", header);
                        resp.status = std::stoi(match[1]);
                        resp.statusMessage = match[2];
                        continue;
                    }

                    if (header.empty()) {
                        continue;
                    }
                    std::pair<std::string, std::string> split = toolboxpp::strings::splitPair(header, ':');
                    std::string leftCopy = boost::algorithm::trim_left_copy(split.first);
                    std::string rightCopy = boost::algorithm::trim_left_copy(split.second);
                    split.first = leftCopy;
                    split.second = rightCopy;

                    if (leftCopy.empty() || rightCopy.empty()) {
                        continue;
                    }

                    resp.addHeader(std::move(split));
                }

                resp._headersBuffer.clear();
            }

            curl_easy_cleanup(curl);
        } else {
            resp.status = -1;
            resp.statusMessage = "CURL error: " + std::string(curl_easy_strerror(res));
        }

        return resp;
    }

    void setConnectionTimeout(long timeoutSeconds) {
        connectionTimeout = timeoutSeconds;
    }
};

}
}

#endif //WSSERVER_STANDALONE_HTTPCLIENT_H
