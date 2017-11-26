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
#include "../defs.h"

namespace wss {
namespace web {

typedef std::pair<std::string, std::string> KeyValue;
typedef std::vector<KeyValue> KeyValueVector;

class Request {
 public:
    enum Method {
      GET, POST, PUT, DELETE
    };

 private:
    std::string url;
    std::string body;
    Method method;
    KeyValueVector headers;
    KeyValueVector params;

 public:
    Request() :
        url(),
        body(),
        method(GET) { }
    explicit Request(const std::string &url) :
        url(url),
        body(),
        method(GET) { }

    explicit Request(std::string &&url) :
        url(std::move(url)),
        body(),
        method(GET) { }

    Request &setUrl(const std::string &url) {
        this->url = url;
        return *this;
    }

    Request &setUrl(std::string &&url) {
        this->url = std::move(url);
        return *this;
    }

    Request &setBody(const std::string &body) {
        this->body = body;
        setHeader({"Content-Length", std::to_string(this->body.length())});
        return *this;
    }

    Request &setBody(std::string &&body) {
        this->body = std::move(body);
        setHeader({"Content-Length", std::to_string(this->body.length())});
        return *this;
    }

    Request &setMethod(Method method) {
        this->method = method;
        return *this;
    }

    Request &setHeader(KeyValue &&keyValue) {
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

        return *this;
    }

    Request &addHeader(KeyValue &&keyValue) {
        headers.push_back(std::move(keyValue));
        return *this;
    }

    Request &addParam(KeyValue &&keyValue) {
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

    const char *getBodyC() const {
        const char *out = body.c_str();
        return out;
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

    KeyValueVector getHeaders() const {
        return headers;
    }

    KeyValueVector getParams() const {
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
  KeyValueVector headers;
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

  KeyValueVector parseFormUrlEncode() {
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

  void dump() {
      std::cout << "Response: " << std::endl
                << "  Status: " << status << std::endl
                << " Message: " << statusMessage << std::endl
                << "    Body: " << data << std::endl
                << " Headers:\n";
      for (auto h: headers) {
          std::cout << "\t" << h.first << ": " << h.second << std::endl;
      }
  }

  bool isSuccess() {
      return status >= 200 && status < 400;
  }
};

class HttpClient {
 private:
    bool verbose = false;

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

            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
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
