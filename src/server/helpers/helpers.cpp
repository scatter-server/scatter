/**
 * wsserver
 * DateHelper.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "helpers.h"

boost::posix_time::ptime wss::helpers::parseDate(const std::string &t, const char *format) {
    pt::time_input_facet *timeFacet(new pt::time_input_facet(format));
    std::locale ioFormat = std::locale(std::locale::classic(), timeFacet);

    pt::ptime parsed;
    std::stringstream ss(t);
    ss.imbue(ioFormat);
    ss >> parsed;

    return parsed;
}

boost::posix_time::ptime wss::helpers::parseISODateTime(const std::string &t) {
    return parseDate(t, DATE_TIME_ISO_8601);
}

std::string wss::helpers::formatBoostPTime(const boost::posix_time::ptime &t, const char *format) {
    pt::time_facet *timeFacet = new pt::time_facet(format);
    std::locale ioFormat = std::locale(std::locale::classic(), timeFacet);

    std::stringstream ss;
    ss.imbue(ioFormat);
    ss << t;

    return ss.str();
}
std::string wss::helpers::getNowISODateTime() {
    return formatBoostPTime(pt::second_clock::local_time(), DATE_TIME_ISO_8601);
}

std::string wss::helpers::humanReadableBytes(unsigned long bytes, bool si) {
    using namespace toolboxpp::strings;
    int unit = si ? 1000 : 1024;
    if (bytes < unit) return std::to_string(bytes) + " B";
    auto exp = (std::size_t) (log(bytes) / log(unit));

    std::string units = (si ? "kMGTPE" : "KMGTPE");
    std::string pre = units.substr(exp - 1) + (si ? "" : "i");
    const char *preC = pre.c_str();
    std::stringstream out;

    char buffer[512];
    sprintf(buffer, "%.1f %sB", bytes / pow(unit, exp), preC);

    return std::string(buffer);
}
