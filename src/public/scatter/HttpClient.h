/**
 * scatter_standalone
 * HttpClient.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef SCATTER_STANDALONE_HTTPCLIENT_H
#define SCATTER_STANDALONE_HTTPCLIENT_H

#include <string>
#include <iostream>
#include <istream>
#include <ostream>
#include <boost/algorithm/string/trim.hpp>
#include <toolboxpp.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "StatusCode.h"
#include "ScatterCore.h"
#include "utility.hpp"

namespace wss {
namespace web {

/// \brief Simple std::pair<std::string, std::string>
using KeyValue = std::pair<std::string, std::string>;

/// \brief Simple vector of pairs wss::web::KeyValue
using KeyValueVector = std::vector<KeyValue>;

class SCATTER_EXPORT IOContainer {
 protected:
    KeyValueVector headers;
    std::string body;

 public:
    IOContainer();

    virtual ~IOContainer() = default;

    /// \brief Set request body data
    /// \param body string data for request/response
    void setBody(const std::string &body);

    /// \brief Move request body data
    /// \param body string data for request/response
    void setBody(std::string &&body);

    /// \brief Set header. Overwrites if already contains
    /// \param keyValue std::pair<std::string, std::string>
    void setHeader(KeyValue &&keyValue);

    /// \brief Adds from map new headers values, if some key exists, value will overwrited
    /// \see addHeader(const KeyValue&)
    /// \param map unorderd_map
    void setHeaders(const std::unordered_map<std::string, std::string> &map);

    /// \brief Adds from map new headers values, if some key exists, value will overwrited
    /// \see addHeader(const KeyValue&)
    /// \param mmp
    void setHeaders(wss::utils::CaseInsensitiveMultimap &mmp);

    /// \brief Adds from map new headers values, if some key exists, value will overwrited
    /// \see addHeader(const KeyValue&)
    /// \param mmp
    void setHeaders(const std::unordered_multimap<std::string, std::string> &mmp);

    /// \brief Check for header keys exists
    /// \param name header name. Searching is case insensitive
    /// \return true is key exists
    bool hasHeader(const std::string &name) const;

    /// \brief Search for header and return row as pair: wss::web::KeyValue
    /// \param headerName string. Searching is case insensitive
    /// \return pair wss::web::KeyValue
    std::pair<std::string, std::string> getHeaderPair(const std::string &headerName) const;

    /// \brief Search for header and return it value
    /// \param headerName string. Searching is case insensitive
    /// \return empty string if not found, otherwise copy of origin value
    std::string getHeader(const std::string &headerName) const;

    /// \brief Search for header and compare it value with comparable string
    /// \param headerName string. Searching is case insensitive
    /// \param comparable string to compare with
    /// \return true if header found and equals to comparable, false otherwise
    bool compareHeaderValue(const std::string &headerName, const std::string &comparable) const;

    /// \brief Add header with separate key and value strings. If header exists, value will be ovewrited.
    /// \param key string. Value will be writed in original case
    /// \param value any string
    void addHeader(const std::string &key, const std::string &value);

    /// \brief Add header with pair of key and value. If header exists, value will be ovewrited by new value.
    /// \see wss::web::KeyValue
    /// \param keyValue
    void addHeader(const KeyValue &keyValue);

    /// \brief Move input pair wss::web::KeyValue to header map. If header exists, value will be ovewrited.
    /// \param keyValue
    /// \see wss::web::KeyValue
    void addHeader(KeyValue &&keyValue);

    /// \brief Add headers collection of key and value. If some header exists, value wil be overwrited by new value
    /// \@see KeyValueVector
    /// \param values
    void addHeaders(const KeyValueVector &values);

    /// \brief Get copy of request/response body
    /// \return Copy of body
    std::string getBody() const;

    /// \brief Get copy of request/response body in char*
    /// \return Copy of body
    const char *getBodyC() const;

    /// \brief Check for body is not empty
    /// \return true if !body.empty()
    bool hasBody() const;

    /// \brief Check for header map has at least one value
    /// \return true if map not empty
    bool hasHeaders() const;

    /// \brief Return copy of header map
    /// \see wss::web::KeyValueVector
    /// \see wss::web::keyValue
    /// \return simple vector of pairs std::vector<KeyValue>
    KeyValueVector getHeaders() const;

    /// \brief Glue headers and return list of its.
    /// \return vector of strings:
    /// Example:
    /// {
    ///     "Content-Type: application/json",
    ///     "Connection: keep-alive"
    /// }
    std::vector<std::string> getHeadersGlued() const;
};

class SCATTER_EXPORT Request : public IOContainer {
 public:
    /// \brief Http methods
    enum Method {
      GET, POST, PUT, DELETE, HEAD
    };
 private:
    /// \brief like multimap but vector
    KeyValueVector m_params;
    std::string m_url;
    Method m_method;

 public:
    Request();
    explicit Request(const std::string &url);
    Request(const std::string &url, Method method);
    explicit Request(std::string &&url);
    ~Request() override { };

    /// \brief parse query string to vector<KeyValue>. String must not contains hostname or protocol, only query string.
    /// Example: ?id=1&param=2&someKey=3
    /// Warning! Keys represented as arrays, will not be recognized as arrays, they will stored as multiple values, of one keys,
    /// and if you will try to get param only by key using getParam(const std::string&), method will return only first found value, not all.
    /// \param queryString
    void parseParamsString(const std::string &queryString);

    /// \brief Convert string method name to wss::web::Request::Method
    /// \param methodName case insensitive string
    /// \return if method string will not be recognized, method will return wss::web::Request::Method::GET
    static Method methodFromString(const std::string &methodName);

    /// \brief Convert method to uppercase string
    /// \param methodName
    /// \return http method name
    static std::string methodToString(Method methodName);

    /// \brief Set request url. Required to send request.
    /// \param url Fully qualified url with host and protocol
    void setUrl(const std::string &url);

    /// \brief Move request url. Required to send request.
    /// \param url Fully qualified url with host and protocol
    void setUrl(std::string &&url);

    /// \brief Set request http method
    /// \param method
    void setMethod(Method method);

    /// \brief Add query param key-value wss::web::KeyValue
    /// \param keyValue pair of strings
    void addParam(KeyValue &&keyValue);

    /// \brief Return existed url
    /// \return url string or empty string if did not set
    std::string getUrl() const;

    /// \brief Return Http method
    /// \return
    Method getMethod() const;

    /// \brief Check for at least one query parameter has set
    /// \return
    bool hasParams() const;

    /// \brief Check for parameter exists
    /// \param key query parameter name
    /// \param icase search case sensititvity
    /// \return true if parameter exists
    bool hasParam(const std::string &key, bool icase = true) const;

    /// \brief Return value of query parameter
    /// \param key query parameter name
    /// \param icase search case sensititvity
    /// \return empty string of parameter did not set
    std::string getParam(const std::string &key, bool icase = true) const;

    /// \brief Build passed url with query parameters
    /// \return url with parameters. if url did not set, will return empty string without parameters
    std::string getUrlWithParams() const;

    /// \brief Return copy of passed parameters
    /// \return simple vector with pairs of strings
    KeyValueVector getParams() const;
};

class SCATTER_EXPORT Response : public IOContainer {
 public:
    int status = 200;
    std::string statusMessage;
    std::string data;
    std::string _headersBuffer;

    /// \brief Return json data from body, empty object if can't parse
    /// \return
    nlohmann::json parseJsonBody() const;

    /// \brief Return map of POST body form-url-encoded data
    /// \return
    KeyValueVector parseFormUrlEncode() const;

    /// \brief Print response data to std::cout
    void dump() const;

    /// \brief Check response status  200 <= code < 400
    /// \return
    bool isSuccess() const;
};

/// \brief Simple Http Client based on libcurl
class SCATTER_EXPORT HttpClient {
 private:
    bool m_verbose = false;
    long m_connectionTimeout = 10L;

    static size_t handleResponseData(void *buffer, size_t size, size_t nitems, void *userData) {
        ((Response *) userData)->data.append((char *) buffer, size * nitems);
        return size * nitems;
    }

    static size_t handleResponseHeaders(void *buffer, size_t size, size_t nitems, void *userData) {
        ((Response *) userData)->_headersBuffer.append((char *) buffer, size * nitems);
        return size * nitems;
    }

 public:
    HttpClient();
    ~HttpClient();

    /// \brief Set verbosity mode for curl
    /// \param enable
    void enableVerbose(bool enable);

    /// \brief Set curl connection timeout
    /// \param timeoutSeconds Long seconds
    void setConnectionTimeout(long timeoutSeconds);

    /// \brief Make request using request
    /// \param request wss::web::Request
    /// \return wss::web::Response
    Response execute(const Request &request);

    /// \brief Async request call
    /// \param request
    template<typename Callback = std::function<wss::web::Response>()>
    void executeAsync(const Request &request, const Callback &cb = nullptr);
};

}
}

#endif //SCATTER_STANDALONE_HTTPCLIENT_H
