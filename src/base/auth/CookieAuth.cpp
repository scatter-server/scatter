/*! 
 * wsserver. 2018
 * 
 * \author Eduard Maximovich <edward.vstock@gmail.com>
 * \link https://github.com/edwardstock
 */

#include "CookieAuth.h"
#include "fmt/format.h"

/// COOKIE
wss::CookieAuth::CookieAuth(const std::string &cookieName, const std::string &cookieValue) :
    m_name(cookieName),
    m_value(cookieValue) {
}
wss::CookieAuth::CookieAuth(std::string &&cookieName, std::string &&cookieValue) :
    m_name(std::move(cookieName)),
    m_value(std::move(cookieValue)) {

}
std::string wss::CookieAuth::getType() {
    return "cookie";
}
void wss::CookieAuth::performAuth(wss::web::Request &request) const {
    request.setHeader({"Cookie", fmt::format("{0}={1}", m_name, m_value)});
}
bool wss::CookieAuth::validateAuth(const wss::web::Request &request) const {
    if (!request.hasHeader("cookie")) {
        return false;
    }
    std::string cookieHeader = request.getHeader("cookie");
    std::vector<std::string> cookies = toolboxpp::strings::split(cookieHeader, ";");

    const std::string pattern = "([a-zA-Z0-9-_]+)=([a-zA-Z0-9-_]+)";
    for (auto &cookie: cookies) {
        if (toolboxpp::strings::hasRegex(pattern, cookie)) {
            const auto &match = toolboxpp::strings::matchRegexp(pattern, cookie);
            if (match.size() != 3) {
                continue;
            }

            const std::string n = match[1];
            const std::string v = match[2];

            if (toolboxpp::strings::equalsIgnoreCase(m_name, n) && v.compare(m_value) == 0) {
                return true;
            }
        }
    }

    return false;
}
std::string wss::CookieAuth::getLocalValue() const {
    return m_value;
}
std::string wss::CookieAuth::getRemoteValue(const wss::web::Request &request) const {
    if (!request.hasHeader("cookie")) {
        return "";
    }
    std::string cookieHeader = request.getHeader("cookie");
    std::vector<std::string> cookies = toolboxpp::strings::split(cookieHeader, ";");

    const std::string pattern = "([a-zA-Z0-9-_]+)=([a-zA-Z0-9-_]+)";
    for (auto &cookie: cookies) {
        if (toolboxpp::strings::hasRegex(pattern, cookie)) {
            const auto &match = toolboxpp::strings::matchRegexp(pattern, cookie);
            if (match.size() != 3) {
                continue;
            }

            const std::string n = match[1];
            const std::string v = match[2];
            if (toolboxpp::strings::equalsIgnoreCase(n, m_name)) {
                return v;
            }
        }
    }

    return "";
}
