/**
 * wsserver_standalone
 * HttpClient.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include <thread>
#include <future>
#include "HttpClient.h"
#include "../helpers/helpers.h"

// BASE IO
wss::web::IOContainer::IOContainer() :
    body() { }
void wss::web::IOContainer::setBody(const std::string &body) {
    this->body = body;
    setHeader({"Content-Length", wss::utils::toString(this->body.length())});
}
void wss::web::IOContainer::setBody(std::string &&body) {
    this->body = std::move(body);
    setHeader({"Content-Length", wss::utils::toString(this->body.length())});
}
void wss::web::IOContainer::setHeader(wss::web::KeyValue &&keyValue) {
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
bool wss::web::IOContainer::hasHeader(const std::string &name) const {
    using toolboxpp::strings::equalsIgnoreCase;
    for (auto &h: headers) {
        if (equalsIgnoreCase(h.first, name)) {
            return true;
        }
    }

    return false;
}
std::pair<std::string, std::string> wss::web::IOContainer::getHeaderPair(const std::string &headerName) const {
    using toolboxpp::strings::equalsIgnoreCase;
    for (auto &h: headers) {
        if (equalsIgnoreCase(h.first, headerName)) {
            return {h.first, h.second};
        }
    }

    return {};
}
std::string wss::web::IOContainer::getHeader(const std::string &headerName) const {
    using toolboxpp::strings::equalsIgnoreCase;
    for (auto &h: headers) {
        if (equalsIgnoreCase(h.first, headerName)) {
            return h.second;
        }
    }

    return std::string();
}
bool wss::web::IOContainer::compareHeaderValue(const std::string &headerName, const std::string &comparable) const {
    if (!hasHeader(headerName)) return false;
    return getHeader(headerName) == comparable;
}
void wss::web::IOContainer::addHeader(const std::string &key, const std::string &value) {
    headers.emplace_back(key, value);
}
void wss::web::IOContainer::addHeader(const wss::web::KeyValue &keyValue) {
    headers.push_back(std::move(keyValue));
}
void wss::web::IOContainer::addHeader(wss::web::KeyValue &&keyValue) {
    headers.push_back(std::move(keyValue));
}
void wss::web::IOContainer::addHeaders(const wss::web::KeyValueVector &values) {
    headers.insert(headers.end(), values.begin(), values.end());
}
void wss::web::IOContainer::setHeaders(const std::unordered_map<std::string, std::string> &map) {
    for (auto &h: map) {
        addHeader(h.first, h.second);
    }
}
void wss::web::IOContainer::setHeaders(CaseInsensitiveMultimap &mmp) {
    for (auto &h: mmp) {
        addHeader(h.first, h.second);
    }
}
void wss::web::IOContainer::setHeaders(const std::unordered_multimap<std::string, std::string> &mmp) {
    for (auto &h: mmp) {
        addHeader(h.first, h.second);
    }
}
std::string wss::web::IOContainer::getBody() const {
    return body;
}
const char *wss::web::IOContainer::getBodyC() const {
    const char *out = body.c_str();
    return out;
}

bool wss::web::IOContainer::hasBody() const {
    return !body.empty();
}
bool wss::web::IOContainer::hasHeaders() const {
    return !headers.empty();
}
wss::web::KeyValueVector wss::web::IOContainer::getHeaders() const {
    return headers;
}
std::vector<std::string> wss::web::IOContainer::getHeadersGlued() const {
    std::vector<std::string> out(headers.size());

    std::stringstream ss;
    int i = 0;
    for (auto h: headers) {
        ss << h.first << ": " << h.second;
        out[i] = ss.str();
        ss.str("");
        ss.clear();
        i++;
    }

    return out;
}

// REQUEST
wss::web::Request::Request() : IOContainer(),
                               m_url(),
                               m_method(GET) { }
wss::web::Request::Request(const std::string &url) : IOContainer(),
                                                     m_url(url),
                                                     m_method(GET) { }

wss::web::Request::Request(const std::string &url, wss::web::Request::Method method) :
    IOContainer(),
    m_url(url),
    m_method(method) {

}
wss::web::Request::Request(std::string &&url) : IOContainer(),
                                                m_url(std::move(url)),
                                                m_method(GET) { }

wss::web::Request::Request(const wss::web::HttpRequest &sRequest) : IOContainer(),
                                                                       m_url(sRequest->path),
                                                                       m_method(methodFromString(sRequest->method)) {
    for (auto &h: sRequest->header) {
        addHeader(h.first, h.second);
    }

    parseParamsString(sRequest->query_string);
}
void wss::web::Request::parseParamsString(const std::string &queryString) {
    std::string query = queryString;
    if (query[0] == '?') {
        query = query.substr(1, query.length());
    }

    std::vector<std::string> pairs = toolboxpp::strings::split(query, "&");

    for (const auto &param: pairs) {
        addParam(toolboxpp::strings::splitPair(param, "="));
    }
}
wss::web::Request::Method wss::web::Request::methodFromString(const std::string &methodName) {
    using toolboxpp::strings::equalsIgnoreCase;

    if (equalsIgnoreCase(methodName, "POST")) {
        return Method::POST;
    } else if (equalsIgnoreCase(methodName, "PUT")) {
        return Method::PUT;
    } else if (equalsIgnoreCase(methodName, "DELETE")) {
        return Method::DELETE;
    } else if (equalsIgnoreCase(methodName, "HEAD")) {
        return Method::HEAD;
    }

    return Method::GET;
}
std::string wss::web::Request::methodToString(wss::web::Request::Method methodName) {
    std::string out;

    switch (methodName) {
        case Method::POST:out = "POST";
            break;
        case Method::PUT:out = "PUT";
            break;
        case Method::DELETE:out = "DELETE";
            break;
        case Method::HEAD: out = "HEAD";
            break;

        default:out = "GET";
            break;
    }

    return out;
}
void wss::web::Request::setUrl(const std::string &url) {
    this->m_url = url;
}
void wss::web::Request::setUrl(std::string &&url) {
    this->m_url = std::move(url);
}
void wss::web::Request::setMethod(wss::web::Request::Method method) {
    this->m_method = method;
}
void wss::web::Request::addParam(wss::web::KeyValue &&keyValue) {
    m_params.push_back(std::move(keyValue));
}
std::string wss::web::Request::getUrl() const {
    return m_url;
}
wss::web::Request::Method wss::web::Request::getMethod() const {
    return m_method;
}
bool wss::web::Request::hasParams() const {
    return !m_params.empty();
}
bool wss::web::Request::hasParam(const std::string &key, bool icase) const {
    using toolboxpp::strings::equalsIgnoreCase;
    const auto &cmp = [icase](const std::string &lhs, const std::string &rhs) {
      if (icase) {
          return equalsIgnoreCase(lhs, rhs);
      } else {
          return lhs == rhs;
      }
    };

    for (const auto &param: m_params) {
        if (cmp(param.first, key)) {
            return true;
        }
    }

    return false;
}
std::string wss::web::Request::getParam(const std::string &key, bool icase) const {
    using toolboxpp::strings::equalsIgnoreCase;
    const auto &cmp = [icase](const std::string &lhs, const std::string &rhs) {
      if (icase) {
          return equalsIgnoreCase(lhs, rhs);
      } else {
          return lhs == rhs;
      }
    };
    for (const auto &param: m_params) {
        if (cmp(param.first, key)) {
            return param.second;
        }
    }

    return std::string();
}
std::string wss::web::Request::getUrlWithParams() const {
    if (m_url.empty()) {
        return std::string();
    }

    std::string combined = m_url;
    if (!m_params.empty()) {
        std::stringstream ss;
        std::vector<std::string> enc;
        for (auto &p: m_params) {
            ss << p.first << "=" << p.second;
            enc.push_back(ss.str());
            ss.str("");
            ss.clear();
        }
        std::string glued = toolboxpp::strings::glue("&", enc);

        if (toolboxpp::strings::hasSubstring(m_url, "?")) {
            combined += "&" + glued;
        } else {
            combined += "?" + glued;
        }
    }

    return combined;
}
wss::web::KeyValueVector wss::web::Request::getParams() const {
    return m_params;
}


// RESPONSE

nlohmann::json wss::web::Response::parseJsonBody() const {
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
wss::web::KeyValueVector wss::web::Response::parseFormUrlEncode() const {
    if (data.empty()) {
        return {};
    }

    std::vector<std::string> groups = toolboxpp::strings::split(data, '&');

    KeyValueVector kvData;
    for (auto &&s: groups) {
        kvData.push_back(toolboxpp::strings::splitPair(s, "="));
    }

    return kvData;
}
void wss::web::Response::dump() const {
    std::cout << "Response: " << std::endl
              << "  Status: " << status << std::endl
              << " Message: " << statusMessage << std::endl
              << "    Body: " << data << std::endl
              << " Headers:\n";
    for (auto h: headers) {
        std::cout << "\t" << h.first << ": " << h.second << std::endl;
    }
}
bool wss::web::Response::isSuccess() const {
    return status >= 200 && status < 400;
}


// CLIENT

wss::web::HttpClient::HttpClient() {
    curl_global_init(CURL_GLOBAL_ALL);
}
wss::web::HttpClient::~HttpClient() {
    curl_global_cleanup();
}
void wss::web::HttpClient::enableVerbose(bool enable) {
    m_verbose = enable;
}

template<typename Callback>
void wss::web::HttpClient::executeAsync(const wss::web::Request &request, const Callback &cb) {
    std::async(std::launch::async, [this, request, cb] {
      auto response = execute(request);
      if (cb) {
          cb(response);
      }
    });
}
wss::web::Response wss::web::HttpClient::execute(const wss::web::Request &request) {
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
            case Request::Method::HEAD:curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "HEAD");
                isPost = false;
                break;
            default:break;
        }

        if (isPost && request.hasBody()) {
            const char *body = request.getBodyC();

            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);

            if (body == nullptr) {
                resp.status = -1;
                resp.statusMessage = "Request body is NULL";
                return resp;
            }
        }

        curl_easy_setopt(curl, CURLOPT_TIMEOUT, m_connectionTimeout);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &HttpClient::handleResponseData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &HttpClient::handleResponseHeaders);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &resp);

        if (request.hasHeaders()) {
            struct curl_slist *headers = nullptr;
            for (const auto &h: request.getHeadersGlued()) {
                if (m_verbose) {
                    L_DEBUG_F("Http::Request", "Header -> %s", h.c_str());
                }

                headers = curl_slist_append(headers, h.c_str());
            }

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }

        if (m_verbose) {
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        }

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            resp.status = -1;
            resp.statusMessage = "CURL error: " + std::string(curl_easy_strerror(res));
        } else {
            std::vector<std::string> headerLines = toolboxpp::strings::split(resp._headersBuffer, "\r\n");
            for (auto &header: headerLines) {
                if (header.length() == 0) {
                    continue;
                }
                if (toolboxpp::strings::hasRegex("HTTP", header)) {
                    std::vector<std::string>
                        match = toolboxpp::strings::matchRegexp(R"(HTTP\/\d\.\d.(\d+).(.*))", header);
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
void wss::web::HttpClient::setConnectionTimeout(long timeoutSeconds) {
    m_connectionTimeout = timeoutSeconds;
}
