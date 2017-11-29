/**
 * wsserver
 * Auth.hpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_AUTH_HPP
#define WSSERVER_AUTH_HPP

#include <string>
#include <sstream>
#include <memory>
#include "json.hpp"
#include "../web/HttpClient.h"
#include "../helpers/base64.h"

namespace wss {
/**
 * available:
 * type: basic
 *      user: basic_username
 *      password: basic_password
 *
 * type: header,
 *      name: header_name
 *      value: header_value
 *
 * type: bearer
 *      value: bearer_token
 */
class WebAuth {
 public:
    /**
     * Auth type
     * @return
     */
    virtual std::string getType();

    /**
     * Set required auth data to request
     */
    virtual void performAuth(wss::web::Request &);

    /**
     * Validate responsed auth data
     * @return
     */
    virtual bool validateAuth(wss::web::Response &);

 protected:
    /**
     * Prepare value for send/validate
     * @return
     */
    virtual std::string getValue() const;
};

/**
 * Basic web authorization with base64(username:password) value
 */
class BasicAuth : public WebAuth {
 public:
    BasicAuth(std::string &&username, std::string &&password);
    BasicAuth(const std::string &username, const std::string &password);

    std::string getType() override;
    void performAuth(wss::web::Request &request) override;
    bool validateAuth(wss::web::Response &response) override;
 protected:
    std::string getValue() const override;
 private:
    std::string username, password;
};

/**
 * Simple header authorization, using custom header value
 */
class HeaderAuth : public WebAuth {
 public:
    HeaderAuth(std::string &&headerName, std::string &&value);
    HeaderAuth(const std::string &headerName, const std::string &value);

    std::string getType() override;
    void performAuth(wss::web::Request &request) override;
    bool validateAuth(wss::web::Response &response) override;
 protected:
    std::string getValue() const override;
 private:
    std::string name, value;
};

/**
 * OAuth bearer authorization, using header "Authorization: Bearer {token}"
 */
class BearerAuth : public HeaderAuth {
 public:
    explicit BearerAuth(std::string &&value);

    std::string getType() override;
    void performAuth(wss::web::Request &request) override;
    bool validateAuth(wss::web::Response &response) override;
 protected:
    std::string getValue() const override;
};

namespace auth {
std::unique_ptr<wss::WebAuth> createFromConfig(const nlohmann::json &config);
}

}

#endif //WSSERVER_AUTH_HPP
