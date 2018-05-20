/**
 * wsserver
 * base64.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 * @copyright 2017 GNU General Public License v3
 */



#ifndef WSSERVER_BASE64_H
#define WSSERVER_BASE64_H

#include <string>

namespace wss {
namespace helpers {

/// \brief Encode string to base 64 string
/// \param len input string length
/// \return encoded string
std::string base64_encode(unsigned char const *, unsigned int len);

/// \brief Decode base64 string
/// \param s input string
/// \return decoded string
std::string base64_decode(std::string const &s);
}
}

#endif //WSSERVER_BASE64_H
