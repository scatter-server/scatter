/*!
 * scatter-server.
 * AMQPTarget.cpp
 *
 * \date 2018
 * \author Eduard Maximovich (edward.vstock@gmail.com)
 * \link   https://github.com/edwardstock
 */

#include <atomic>
#include <openssl/ssl.h>
#include <boost/chrono.hpp>
#include "AMQPTarget.h"

const std::unordered_map<std::string, AMQP::ExchangeType> wss::event::amqp::AMQP_EXCHANGE_TYPE_MAP = {
    {"direct",          AMQP::ExchangeType::direct},
    {"fanout",          AMQP::ExchangeType::fanout},
    {"topic",           AMQP::ExchangeType::topic},
    {"headers",         AMQP::ExchangeType::headers},
    {"consistent_hash", AMQP::ExchangeType::consistent_hash}
};

const std::unordered_map<std::string, int> wss::event::amqp::AMQP_FLAG_MAP = {
    {"durable",     AMQP::durable},
    {"active",      AMQP::active},
    {"passive",     AMQP::passive},
    {"ifunused",    AMQP::ifunused},
    {"ifempty",     AMQP::ifempty},
    {"global",      AMQP::global},
    {"nolocal",     AMQP::nolocal},
    {"noack",       AMQP::noack},
    {"exclusive",   AMQP::exclusive},
    {"nowait",      AMQP::nowait},
    {"mandatory",   AMQP::mandatory},
    {"immediate",   AMQP::immediate},
    {"redelivered", AMQP::redelivered},
    {"multiple",    AMQP::multiple},
    {"requeue",     AMQP::requeue},
    {"readable",    AMQP::readable},
    {"writable",    AMQP::writable},
    {"internal",    AMQP::internal},
};

void wss::event::amqp::from_json(const nlohmann::json &j, wss::event::amqp::AMQPConfig &cfg) {
    cfg.host = j.value("host", "localhost");
    if (j.find("port") != j.end() && !j.at("port").is_null()) {
        cfg.port = (uint16_t) j.at("port").get<uint16_t>();
    }

    cfg.ssl = j.value("ssl", false);
    cfg.vhost = j.value("vhost", "/");
    cfg.user = j.value("user", "guest");
    cfg.password = j.value("password", "guest");

    cfg.exchange = j.value("exchange", AMQPExchange{"scatter_chat_direct", "direct", "durable"});
    cfg.queue = j.value("queue", AMQPQueue{"scatter_chat", "durable"});

    if (j.find("routingKey") == j.end() || j.at("routingKey").is_null()) {
        cfg.routingKey = cfg.queue.name;
    } else {
        cfg.routingKey = j.at("routingKey");
    }

    if (j.find("messageFlags") == j.end() || j.at("messageFlags").is_null()) {
        cfg.messageFlags = std::string();
    } else {
        cfg.messageFlags = j.at("messageFlags").get<std::string>();
    }
}

void wss::event::amqp::from_json(const nlohmann::json &j, wss::event::amqp::AMQPQueue &queue) {
    queue.name = j.value("name", "scatter_chat");
    queue.flags = j.value("flags", "durable");
}

void wss::event::amqp::from_json(const nlohmann::json &j, wss::event::amqp::AMQPExchange &exchange) {
    exchange.name = j.value("name", "scatter_chat_direct");
    exchange.flags = j.value("flags", "durable");
    exchange.type = j.value("type", "direct");
}

wss::event::amqp::AMQPTarget::AMQPTarget(const nlohmann::json &config) :
    Target(config),
    m_onError(std::bind(&AMQPTarget::onConnectionError, this, std::placeholders::_1, std::placeholders::_2)),
    m_running(true),
    m_cfg() {

    m_cfg = config;

    // https://github.com/CopernicaMarketingSoftware/AMQP-CPP/tree/v4.0.1

    if (m_cfg.ssl) {
        OPENSSL_init_ssl(0, NULL);
    }

    m_heartbeatThread = new boost::thread([this]() {
      while (m_running) {
          {
              wss::Logger::get().warning(__FILE__, __LINE__, "AMQP", "Wait for heartbeat...");
              m_waitForHeartbeat = true;
              std::lock_guard<std::mutex> lock(m_connectionMutex);
              if (m_connection && m_connection->usable()) {
                  wss::Logger::get().warning(__FILE__, __LINE__, "AMQP", "Heartbeating!");
                  m_connection->heartbeat();
                  m_waitForHeartbeat = false;
              }
          }

          boost::this_thread::sleep_for(boost::chrono::seconds(10));
      }
    });

    start();
}
wss::event::amqp::AMQPTarget::~AMQPTarget() {
    stop();
    m_running = false;
    m_heartbeatThread->join();
    delete m_heartbeatThread;
}

void wss::event::amqp::AMQPTarget::onConnectionError(AMQP::TcpConnection */*conn*/, const char *errorMessage) {
    wss::Logger::get().warning(__FILE__, __LINE__, "AMQP", fmt::format("AMQP connection error: {0}", errorMessage));
    setValidState(false);
    boost::this_thread::sleep_for(boost::chrono::seconds(5));

    try {
        {
            wss::Logger::get().debug(__FILE__, __LINE__, "AMQP", "Reconnecting...");
            stop();
            start();
        }

    } catch (const std::exception &e) {
        wss::Logger::get()
            .warning(__FILE__, __LINE__, "AMQP", fmt::format("Unable to restart AMQP connection: {0}", e.what()));
        throw std::runtime_error(fmt::format("Unable to restart AMQP connection: {0}", e.what()));
    }
}

void wss::event::amqp::AMQPTarget::start() {
    std::lock_guard<std::mutex> lock(m_connectionMutex);

    m_connectionService = new boost::asio::io_service();
    m_handler = new amqp::ScatterBoostAsioHandler(*m_connectionService);
    m_handler->setOnErrorListener(m_onError);

    // make a connection
    m_connection = std::make_unique<AMQP::TcpConnection>(m_handler, m_cfg.buildAddress());

    // we need a channel too
    m_channel = std::make_unique<AMQP::TcpChannel>(m_connection.get());

    m_channel->declareExchange(m_cfg.exchange.name, m_cfg.exchange.getType(), m_cfg.exchange.getFlags())
             .onSuccess([this]() {
               wss::Logger::get().info(__FILE__,
                                       __LINE__,
                                       "AMQP",
                                       fmt::format("Declare: exchange={0}, type={1}, flags={2}",
                                                   m_cfg.exchange.name,
                                                   m_cfg.exchange.type,
                                                   m_cfg.exchange.flags)
               );
             })
             .onError([this](const char *msg) {
               appendErrorMessage(std::string(msg));
               wss::Logger::get()
                   .debug(__FILE__, __LINE__, "AMQP", fmt::format("Unable to declare exchange: {0}", msg));
             });

    // create a temporary queue
    m_channel->declareQueue(m_cfg.queue.name, m_cfg.queue.getFlags())
             .onSuccess([this](const std::string &name, uint32_t messagecount, uint32_t consumercount) {
               wss::Logger::get().info(__FILE__,
                                       __LINE__,
                                       "AMQP",
                                       fmt::format("Declare: queue={0}, flags={1}, consumer_tag={2}, mcnt={3}, cc={4}",
                                                   m_cfg.queue.name,
                                                   m_cfg.queue.flags,
                                                   name, messagecount, consumercount));
             })
             .onError([this](const char *msg) {
               appendErrorMessage(std::string(msg));
               wss::Logger::get()
                   .error(__FILE__, __LINE__, "AMQP", fmt::format("Unable to declare queue: {0}", msg));
             });

    m_channel->bindQueue(m_cfg.exchange.name, m_cfg.queue.name, m_cfg.routingKey)
             .onSuccess([this]() {
               wss::Logger::get().info(__FILE__,
                                       __LINE__,
                                       "AMQP",
                                       fmt::format("Bind: queue={0}, exchange={1}, routing_key={2}",
                                                   m_cfg.queue.name, m_cfg.exchange.name, m_cfg.routingKey));

               setValidState(true);
             })
             .onError([this](const char *err) {
               appendErrorMessage(std::string(err));
               wss::Logger::get()
                   .error(__FILE__, __LINE__, "AMQP", fmt::format("Unable to bind queue: {0}", err));
             });

    m_workerThread = new boost::thread([this]() {
      m_connectionService->run();
    });
}

void wss::event::amqp::AMQPTarget::stop() {
    std::lock_guard<std::mutex> lock(m_connectionMutex);
    m_connection->close();
    m_channel = nullptr;
    m_connection = nullptr;
    delete m_handler;
    m_connectionService->stop();
    m_connectionService = nullptr;

    if (m_workerThread) {
        m_workerThread->interrupt();
    }
    delete m_workerThread;

    setValidState(false);
}

// now, only sequantially
// @TODO make parallel send, probably batched
void wss::event::amqp::AMQPTarget::send(const wss::MessagePayload &payload,
                                        wss::event::Target::OnSendSuccess successCallback,
                                        wss::event::Target::OnSendError errorCallback) const {

    int maxTries = 3;
    int tries = 0;

//    std::lock_guard<std::mutex> lock(m_connectionMutex);
//    m_connectionMutex.lock();

    std::unique_lock<std::mutex> cvLock(m_waitMutex);
    std::condition_variable cv;
    std::atomic_bool complete;
    complete = false;

    try {
        m_channel->startTransaction();
        const std::string pl = payload.toJson();
        const char *message = pl.c_str();
        m_channel->publish(
            m_cfg.exchange.name,
            m_cfg.routingKey,
            message,
            m_cfg.getMessageFlags()
        );

        m_channel->commitTransaction()
                 .onSuccess(successCallback)
                 .onError(errorCallback)
                 .onFinalize([&cv, &complete, this] {
                   std::unique_lock<std::mutex> cvLock(m_waitMutex);
                   complete = true;
                   cv.notify_one();
                 });


    } catch (const std::exception &e) {
        errorCallback(e.what());
        complete = true;
    }

    while (!complete) {
        cv.wait_for(cvLock, std::chrono::seconds(2));
        tries++;
        if (tries >= maxTries) {
            complete = true;
            errorCallback("Timed out (15 seconds)");
        }
    }
    if (m_waitForHeartbeat) {
        m_connection->heartbeat();
        wss::Logger::get().warning(__FILE__, __LINE__, "AMQP", "Heartbeating (while sending)!");
    }

    Logger::get().debug(__FILE__, __LINE__, "AMQP", "Send complete");

}

std::string wss::event::amqp::AMQPTarget::getType() {
    return "amqp";
}

wss::event::Target *wss::event::amqp::target_create(const nlohmann::json &config) {
    return new AMQPTarget(config);
}

void wss::event::amqp::target_release(wss::event::Target *target) {
    delete target;
}
