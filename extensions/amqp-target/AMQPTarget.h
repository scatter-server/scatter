/*!
 * scatter-server.
 * AMQPTarget.h
 *
 * \date 2018
 * \author Eduard Maximovich (edward.vstock@gmail.com)
 * \link   https://github.com/edwardstock
 */

#ifndef SCATTER_SERVER_AMQPTARGET_H
#define SCATTER_SERVER_AMQPTARGET_H

#include <scatter/Target.h>
#include <sstream>
#include <memory>
#include <iostream>
#include <string>
#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>
#include <amqpcpp/linux_tcp/tcpchannel.h>
#include <amqpcpp/libboostasio.h>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <toolboxpp.h>
#include <mutex>

namespace wss {
namespace event {

namespace amqp {

extern const std::unordered_map<std::string, AMQP::ExchangeType> AMQP_EXCHANGE_TYPE_MAP;
extern const std::unordered_map<std::string, int> AMQP_FLAG_MAP;

AMQP::ExchangeType stringToExchangeType(const std::string &type) {
    if (!AMQP_EXCHANGE_TYPE_MAP.count(type)) {
        return AMQP::ExchangeType::direct;
    }

    return AMQP_EXCHANGE_TYPE_MAP.at(type);
}

int stringToAmqpFlags(const std::string &flags) {
    if (flags.empty()) {
        return 0;
    }

    int out = 0;

    std::vector<std::string> sFlags = toolboxpp::strings::split(flags, "|");
    for (const std::string &f: sFlags) {
        if (AMQP_FLAG_MAP.count(f)) {
            out |= AMQP_FLAG_MAP.at(f);
        }
    }

    return out;
}

struct AMQPQueue {
  std::string name;
  std::string flags;
  int getFlags() const {
      return stringToAmqpFlags(flags);
  }

  friend void from_json(const nlohmann::json &j, AMQPQueue &cfg);
};

struct AMQPExchange {
  std::string name;
  std::string type;
  std::string flags;
  int getFlags() const {
      return stringToAmqpFlags(flags);
  }

  AMQP::ExchangeType getType() {
      return stringToExchangeType(type);
  }

  friend void from_json(const nlohmann::json &j, AMQPExchange &cfg);
};

struct AMQPConfig {
  std::string host = "localhost";
  uint16_t port = 5672;
  bool ssl = false;
  std::string vhost = "/";
  std::string user = "guest";
  std::string password = "guest";
  std::string routingKey;
  AMQPQueue queue = AMQPQueue{"scatter_event", "durable"};
  AMQPExchange exchange = AMQPExchange{"scatter_exchange_direct", "type", "durable"};
  std::string messageFlags;

  int getMessageFlags() const {
      if (messageFlags.empty()) {
          return 0;
      }
      return stringToAmqpFlags(messageFlags);
  }

  const AMQP::Address buildAddress() {
      std::stringstream ss;
      if (ssl) {
          ss << "amqps";
      } else {
          ss << "amqp";
      }

      ss << "://";
      ss << user << ":" << password;
      ss << "@";
      ss << host << ":" << port;
      ss << vhost;

      return AMQP::Address(ss.str());
  }

  friend void from_json(const nlohmann::json &j, AMQPConfig &cfg);
};
}

class AMQPTarget : public wss::event::Target {
 public:
    explicit AMQPTarget(const json &config);
    virtual ~AMQPTarget();

    bool send(const wss::MessagePayload &payload, std::string &error) override;
    std::string getType() override;

 private:
    bool success;
    std::mutex m_sendLock;

    amqp::AMQPConfig *cfg;
    boost::asio::io_service service;
    AMQP::LibBoostAsioHandler handler;
    std::unique_ptr<AMQP::TcpConnection> connection;
    std::unique_ptr<AMQP::TcpChannel> channel;
    boost::thread *publishService;

    void stop();
};

extern "C" SCATTER_EXPORT Target *target_create(const nlohmann::json &config);
extern "C" SCATTER_EXPORT void target_release(Target *target);
}
}

void from_json(const nlohmann::json &j, wss::event::amqp::AMQPConfig &cfg);
void to_json(nlohmann::json &j, const wss::event::amqp::AMQPConfig &cfg);

#endif //SCATTER_SERVER_AMQPTARGET_H
