/*!
 * wsserver
 * MessageID.h
 *
 * \date   2017
 * \author Eduard Maximovich (edward.vstock@gmail.com)
 * \link   https://github.com/edwardstock
 */

#ifndef WSSERVER_UNID_H
#define WSSERVER_UNID_H

#include <string>
#include <json.hpp>
#include <fmt/format.h>

namespace wss {

/// \brief Unique ID generator
/// Id contains:
/// 4 bytes - current unix timestamp (seconds since 1970)
/// 4 bytes - 4 random bytes (of 16) from uuid
/// 2 bytes - current process PID. If pid is 32 bit, it will cutted to 16 bits by: pid & 0xFFFF
/// 4 bytes - incremental integer
class unid {
 public:
    struct id {
      uint32_t tm;
      uint32_t uuid;
      uint16_t pid;
      uint32_t inc;
      const std::string str() const {
          return fmt::format("{:08X}-{:08X}-{:04X}-{:08X}", tm, uuid, pid, inc);
      }

      friend std::ostream &operator<<(std::ostream &os, const unid::id &other) noexcept {
          os << other.str();
          return os;
      }

      inline bool operator==(const unid::id &other) const {
          return tm == other.tm &&
              uuid == other.uuid &&
              pid == other.pid &&
              inc == other.inc;
      }

      friend void to_json(nlohmann::json &obj, const unid::id &id) {
          obj = id.str();
      }
    };

    static unid &generator() {
        static unid mid;
        return mid;
    }

    /// \brief Helper operator to use with object reference.
    /// Example: auto &generate = wss::unit::generator();
    /// wss::unid_t id = generate();
    /// \return
    unid::id operator()() noexcept;

    /// \brief Generates next identifier
    /// \return String identifier. Example: 5A32EA72-38F4D5DC-3541-00000008
    unid::id next();

 private:
    /// \brief Just for init variables
    unid();

    /// \brief Generates new uuid and set new value to uuidBytes
    void generateUUID();

    /// \brief Maximum auto increment value
    const uint32_t maxCounter = 0xFFFFFFFF - 1;
    /// \brief Number of when we'll generate new UUID
    const uint32_t maxCounterUUID = 1000;

    /// \brief PID 2 bytes usual (max 65535)
    uint16_t pid;
    /// \brief First 4 random bytes from uuid
    std::atomic<uint32_t> uuidBytes;
    /// \brief Incremental counter
    std::atomic<uint32_t> counter;

};

/// \brief ID type alias
using unid_t = unid::id;
}

#endif //WSSERVER_UNID_H
