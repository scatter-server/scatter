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
#include "QueryBuilder.hpp"
#include <boost/algorithm/string/trim.hpp>

namespace wss {
namespace web {

class Request {
 public:
    enum Method {
      GET, POST, PUT, DELETE
    };

 private:
    std::string url;
    std::string body;
    Method method;
    std::vector<std::pair<std::string, std::string>> headers;
    std::vector<std::pair<std::string, std::string>> params;

 public:
    explicit Request(const std::string &url) :
        url(url),
        method(GET),
        body() {
    }

    explicit Request(std::string &&url) :
        url(std::move(url)),
        method(GET),
        body() {
    }

    Request &setBody(const std::string &body) {
        this->body = body;
        return *this;
    }

    Request &setBody(std::string &&body) {
        this->body = std::move(body);
        return *this;
    }

    Request &setMethod(Method method) {
        this->method = method;
        return *this;
    }

    Request &addHeader(std::pair<std::string, std::string> &&keyValue) {
        headers.push_back(std::move(keyValue));
        return *this;
    }

    Request &addParam(std::pair<std::string, std::string> &&keyValue) {
        params.push_back(std::move(keyValue));
        return *this;
    }

    std::string getUrl() const {
        return url;
    }

    Method getMethod() const {
        return method;
    }

    bool hasBody() const {
        return !body.empty();
    }

    bool hasHeaders() const {
        return !headers.empty();
    }

    bool hasParams() const {
        return !params.empty();
    }

    std::string getBody() const {
        return body;
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

    std::vector<std::pair<std::string, std::string>> getHeaders() const {
        return headers;
    }

    std::vector<std::pair<std::string, std::string>> getParams() const {
        return params;
    }

    std::vector<std::string> getHeadersGlued() const {
        std::vector<std::string> out;
        for (auto &h: headers) {
            out.emplace_back(h.first + ": " + h.second);
        }

        return out;
    }
};

struct Response {
  int status;
  std::string statusMessage;
  std::string data;
  std::vector<std::pair<std::string, std::string>> headers;
  std::string _headersBuffer;

  bool isSuccess() {
      return status >= 200 && status < 400;
  }
};

class HttpClient {
 private:

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

    Response execute(const Request &request) {
        CURL *curl;
        CURLcode res;

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
                default:break;
            }

            if (isPost && request.hasBody()) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.getBody().c_str());
            }

            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &HttpClient::handleResponseData);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &HttpClient::handleResponseHeaders);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &resp);

            if (request.hasHeaders()) {
                struct curl_slist *headers = nullptr;
                for (auto &h: request.getHeadersGlued()) {
                    curl_slist_append(headers, h.c_str());
                }

                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
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

                    resp.headers.push_back(split);
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
};

}
}

#endif //WSSERVER_STANDALONE_HTTPCLIENT_H
