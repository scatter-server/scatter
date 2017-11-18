/**
 * wsserver
 * DateHelper.hpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_DATEHELPER_HPP
#define WSSERVER_DATEHELPER_HPP

#include <string>
#include <ctime>
#include <boost/date_time.hpp>
#include <chrono>
#include <unordered_map>

namespace wss {
namespace helpers {

namespace cal = boost::gregorian;
namespace pt = boost::posix_time;

using namespace std::chrono;

typedef std::chrono::high_resolution_clock::time_point hires_time_t;
using hires_clock = std::chrono::high_resolution_clock;

/**
 * ISO 8601 date format without time zone at the end
 */
static const char *DATE_TIME_ISO_8601 = "%Y-%m-%d %H:%M:%S";

pt::ptime parseDate(const std::string &t, const char *format);
/**
 *
 * @param t string: 2017-11-15 23:59:59
 * @return boost ptime
 */
pt::ptime parseISODateTime(const std::string &t);

/**
 * @param t boost::posix_time::ptime
 * @param format string format.
 * @link http://www.boost.org/doc/libs/1_55_0/doc/html/date_time/date_time_io.html
 * @return
 */
std::string formatBoostPTime(const pt::ptime &t, const char *format);

/**
 * Current date time
 * @return
 */
std::string getNowISODateTime();

};
};

#endif //WSSERVER_DATEHELPER_HPP
