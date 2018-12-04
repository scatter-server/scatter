/*!
 * scatter.
 * scatter_core.h
 *
 * \date 2018
 * \author Eduard Maximovich (edward.vstock@gmail.com)
 * \link   https://github.com/edwardstock
 */

#ifndef SCATTER_CORE_H
#define SCATTER_CORE_H

#if defined _WIN32 || defined __CYGWIN__
#ifdef WIN_EXPORT
// Exporting...
#ifdef __GNUC__
#define SCATTER_EXPORT __attribute__ ((dllexport))
#else
#define SCATTER_EXPORT __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
#endif
#else
#ifdef __GNUC__
#define SCATTER_EXPORT __attribute__ ((dllimport))
#else
#define SCATTER_EXPORT __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
#endif
#endif
#define SCATTER_NO_EXPORT
#else
#if __GNUC__ >= 4
#define SCATTER_EXPORT __attribute__ ((visibility ("default")))
#define SCATTER_NO_EXPORT  __attribute__ ((visibility ("hidden")))
#else
#define SCATTER_EXPORT
#define SCATTER_NO_EXPORT
#endif
#endif

namespace wss {

using user_id_t = unsigned long;
using conn_id_t = unsigned long;

template<typename T>
using UserMap = std::unordered_map<user_id_t, T>;

template<typename T>
using ConnectionMap = std::unordered_map<conn_id_t, T>;

using json = nlohmann::json;

}

#endif //SCATTER_CORE_H
