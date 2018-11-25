/**
 * scatter
 * DateHelper.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "../base/Settings.hpp"
#include "helpers.h"
#include "date/date.h"
#include "date/tz.h"

boost::posix_time::ptime wss::utils::parseDate(const std::string &t, const char *format) {
    pt::time_input_facet *timeFacet(new pt::time_input_facet(format));
    std::locale ioFormat = std::locale(std::locale::classic(), timeFacet);

    pt::ptime parsed;
    std::stringstream ss(t);
    ss.imbue(ioFormat);
    ss >> parsed;

    return parsed;
}

boost::posix_time::ptime wss::utils::parseISODateTime(const std::string &t) {
    return parseDate(t, DATE_TIME_ISO_8601);
}

std::string wss::utils::formatBoostPTime(const boost::posix_time::ptime &t, const char *format) {
    pt::time_facet *timeFacet = new pt::time_facet(format);
    std::locale ioFormat = std::locale(std::locale::classic(), timeFacet);

    std::stringstream ss;
    ss.imbue(ioFormat);
    ss << t;

    delete timeFacet;

    return ss.str();
}

std::string wss::utils::getNowISODateTimeFractionalConfigAware() {
    if (wss::Settings::get().server.timezone.empty()) {
        return getNowISODateTime("UTC");
    }

    return getNowISODateTime(wss::Settings::get().server.timezone);
}

std::string wss::utils::getNowLocalISODateTime() {
    return formatBoostPTime(pt::second_clock::local_time(), DATE_TIME_ISO_8601);
}

std::string wss::utils::getNowLocalISODateTimeFractional() {
    return formatBoostPTime(pt::microsec_clock::local_time(), DATE_TIME_ISO_8601_FRACTIONAL);
}

std::string wss::utils::getNowLocalISODateTimeFractionalTZ() {
    auto t = date::make_zoned(date::current_zone(), std::chrono::system_clock::now());

    return date::format(wss::utils::DATE_TIME_ISO_8601_FRACTIONAL_WITH_TZ, t);
}

std::string wss::utils::getNowUTCISODateTime() {
    return formatBoostPTime(pt::second_clock::universal_time(), DATE_TIME_ISO_8601);
}

std::string wss::utils::getNowISODateTime(const std::string &timezone) {
    // @TODO hack!
    auto t = date::make_zoned(timezone, std::chrono::system_clock::now());
    const std::string f1 = "%Y-%m-%d %H:%M:";
    const std::string f2 = "%S";
    const std::string f3 = "%Oz";

    std::string out = date::format(f1, t);
    std::string s = date::format(f2, t);
    // we need to reduce nanoseconds precision, debian stretch give 9 numbers after point instead of 6
    if (s.length() > 9) {
        s = s.substr(0, 9);
    }
    std::string tz = date::format(f3, t);
    out += s;
    out += tz;

    return out;
}

std::string wss::utils::getNowUTCISODateTimeFractional() {
    return formatBoostPTime(pt::microsec_clock::universal_time(), DATE_TIME_ISO_8601_FRACTIONAL);
}

std::string wss::utils::getNowUTCISODateTimeFractionalTZ() {
    auto t = date::make_zoned("UTC", std::chrono::system_clock::now());
    return date::format(wss::utils::DATE_TIME_ISO_8601_FRACTIONAL_WITH_TZ, t);
}

std::string wss::utils::humanReadableBytes(unsigned long bytes, bool si) {
    using namespace toolboxpp::strings;
    unsigned int unit = si ? 1000 : 1024;
    if (bytes < unit) return wss::utils::toString(bytes) + " B";
    auto exp = (std::size_t) (log(bytes) / log(unit));

    std::string units = (si ? "kMGTPE" : "KMGTPE");
    std::string pre = units.substr(exp - 1) + (si ? "" : "i");
    const char *preC = pre.c_str();
    std::stringstream out;

    char buffer[512];
    sprintf(buffer, "%.1f %sB", bytes / pow(unit, exp), preC);

    return std::string(buffer);
}

const std::string wss::utils::generateRandomStringCRC32(unsigned short length) {
    std::string chars(
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "1234567890"
        "!@#$%^&*()"
        "`~-_=+[{]}\\|;:'\",<.>/? ");

    boost::random::random_device rng;
    boost::random::uniform_int_distribution<> index_dist(0, static_cast<int>(chars.size() - 1));
    std::stringstream ss;
    for (int i = 0; i < length; ++i) {
        ss << chars[index_dist(rng)];
    }

    const std::string randomed = ss.str();
    boost::crc_32_type result;
    result.process_bytes(randomed.data(), randomed.length());

    return wss::utils::toString(result.checksum());
}
