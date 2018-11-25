/*!
 * scatter
 * TestAuth.cpp
 *
 * \date   2017
 * \author Eduard Maximovich (edward.vstock@gmail.com)
 * \link   https://github.com/edwardstock
 */

#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include <src/server/web/HttpClient.h>
#include <src/server/base/Auth.h>
#include <src/server/helpers/base64.h>

#include "gtest/gtest.h"

TEST(AuthTest, NoAuthTest) {
    wss::web::Request request;
    wss::WebAuth webAuth;

    ASSERT_STREQ(webAuth.getType().c_str(), "noauth");
    webAuth.performAuth(request);
    ASSERT_FALSE(request.hasParams()); // no params
    ASSERT_TRUE(request.getHeaders().empty()); // no headers
    ASSERT_TRUE(request.getBody().empty()); // no body

    ASSERT_TRUE(webAuth.validateAuth(request));
}

TEST(AuthTest, BasicAuthTest) {
    wss::web::Request request;
    const std::string username = "username";
    const std::string password = "password";
    wss::BasicAuth basicAuth(username, password);

    ASSERT_STREQ(basicAuth.getType().c_str(), "basic");
    basicAuth.performAuth(request);

    std::stringstream ss;
    ss << username << ":" << password;
    const std::string glued = ss.str();
    const std::string encoded = wss::helpers::base64_encode(
        reinterpret_cast<const unsigned char *>(glued.c_str()),
        static_cast<unsigned int>(glued.length())
    );
    const std::string value = "Basic " + encoded;

    ASSERT_TRUE(request.hasHeader("Authorization"));
    ASSERT_TRUE(request.hasHeader("authorization")); //all compares are case insensitive

    auto header = request.getHeader("authorization");
    ASSERT_STREQ(header.c_str(), value.c_str());

    // reversed
    ASSERT_TRUE(basicAuth.validateAuth(request));

    // empty request
    wss::web::Request emptyRequest;
    ASSERT_FALSE(basicAuth.validateAuth(emptyRequest));
}

TEST(AuthTest, HeaderAuthTest) {
    wss::web::Request request;
    const std::string header = "X-Api-TOKEN";
    const std::string value = "token long long long long token";

    wss::HeaderAuth headerAuth(header, value);

    ASSERT_STREQ(headerAuth.getType().c_str(), "header");

    headerAuth.performAuth(request);

    ASSERT_TRUE(request.hasHeaders());
    ASSERT_STREQ(request.getHeader(header).c_str(), value.c_str());
    ASSERT_STREQ(request.getHeader("x-api-token").c_str(), value.c_str());

    // reversed
    ASSERT_TRUE(headerAuth.validateAuth(request));

    // empty request
    wss::web::Request emptyRequest;
    ASSERT_FALSE(headerAuth.validateAuth(emptyRequest));
}

TEST(AuthTest, BearerAuthTest) {
    wss::web::Request request;
    const std::string header = "Authorization";
    const std::string token = "mytoken";
    const std::string tokenValue = "Bearer " + token;
    wss::BearerAuth bearerAuth(token);

    ASSERT_STREQ(bearerAuth.getType().c_str(), "bearer");

    bearerAuth.performAuth(request);

    ASSERT_TRUE(request.hasHeaders());
    ASSERT_TRUE(request.hasHeader(header));
    // must contains `Bearer` before value
    ASSERT_STRNE(request.getHeader(header).c_str(), token.c_str());
    ASSERT_STREQ(request.getHeader(header).c_str(), tokenValue.c_str());

    ASSERT_TRUE(bearerAuth.validateAuth(request));
    wss::web::Request emptyRequest;
    ASSERT_FALSE(bearerAuth.validateAuth(emptyRequest));
}

TEST(AuthTest, CreateAuthFromConfig) {
    // NO Auth
    std::unique_ptr<wss::WebAuth> noAuth;

    noAuth = wss::auth::createFromConfig({{"type", "noauth"}});
    ASSERT_STREQ(noAuth->getType().c_str(), "noauth");


    // basic auth
    std::unique_ptr<wss::WebAuth> basicAuth;

    bool basicHasError = false;

    try {
        basicAuth = wss::auth::createFromConfig({
                                                    {"type", "basic"},
                                                });
    } catch (const nlohmann::detail::out_of_range &e) {
        basicHasError = true;
    }

    ASSERT_TRUE(basicHasError);

    basicAuth = wss::auth::createFromConfig({
                                                {"type", "basic"},
                                                {"user", "username"},
                                                {"password", "password"}
                                            });

    ASSERT_STREQ(basicAuth->getType().c_str(), "basic");


    // header auth
    wss::web::Request headerRequest;
    std::unique_ptr<wss::WebAuth> headerAuth = wss::auth::createFromConfig(
        {
            {"type", "header"},
            {"name", "header-name"},
            {"value", "header-value"}
        }
    );
    headerAuth->performAuth(headerRequest);
    ASSERT_STREQ(headerAuth->getType().c_str(), "header");
    ASSERT_TRUE(headerRequest.hasHeader("header-name"));
    ASSERT_STREQ(headerRequest.getHeader("header-NAME").c_str(), "header-value");

    // bearer auth
    wss::web::Request bearerRequest;
    std::unique_ptr<wss::WebAuth> bearerAuth = wss::auth::createFromConfig(
        {
            {"type", "bearer"},
            {"value", "value"},
        }
    );
    bearerAuth->performAuth(bearerRequest);
    ASSERT_STREQ(bearerAuth->getType().c_str(), "bearer");
    ASSERT_TRUE(bearerRequest.hasHeader("Authorization"));
    ASSERT_STREQ(bearerRequest.getHeader("Authorization").c_str(), "Bearer value");

    // empty config == noauth
    std::unique_ptr<wss::WebAuth> emptyAuth = wss::auth::createFromConfig({});
    ASSERT_STREQ(emptyAuth->getType().c_str(), "noauth");

    // also support pass single object
    std::unique_ptr<wss::WebAuth> emptyAnotherAuth = wss::auth::createFromConfig(
        {
            {"auth", nlohmann::json {
                {"type", "noauth"},
                {"name", "name"},
                {"value", "value"}
            }}
        }
    );
    ASSERT_STREQ(emptyAnotherAuth->getType().c_str(), "noauth");

}
