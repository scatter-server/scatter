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

wss::event::AMQPTarget::AMQPTarget(const nlohmann::json &config) :
    Target(config),
    cfg(new amqp::AMQPConfig),
    service(2),
    handler(service) {
    *cfg = config;


    // https://github.com/CopernicaMarketingSoftware/AMQP-CPP/tree/v4.0.1

    if (cfg->ssl) {
        OPENSSL_init_ssl(0, NULL);
    }
    // make a connection
    connection = std::make_unique<AMQP::TcpConnection>(&handler, cfg->buildAddress());

    // we need a channel too
    channel = std::make_unique<AMQP::TcpChannel>(connection.get());

    start();
}
wss::event::AMQPTarget::~AMQPTarget() {
    stop();
    delete cfg;
}

void wss::event::AMQPTarget::start() {
    channel->declareExchange(cfg->exchange.name, cfg->exchange.getType(), cfg->exchange.getFlags())
           .onSuccess([this]() {
             wss::Logger::get().info(__FILE__,
                                     __LINE__,
                                     "AMQP",
                                     fmt::format("Declare: exchange={0}, type={1}, flags={2}",
                                                 cfg->exchange.name,
                                                 cfg->exchange.type,
                                                 cfg->exchange.flags)
             );
           })
           .onError([this](const char *msg) {
             appendErrorMessage(std::string(msg));
             wss::Logger::get()
                 .debug(__FILE__, __LINE__, "AMQP", fmt::format("Unable to declare exchange: {0}", msg));
           });

    // create a temporary queue
    channel->declareQueue(cfg->queue.name, cfg->queue.getFlags())
           .onSuccess([this](const std::string &name, uint32_t messagecount, uint32_t consumercount) {
             wss::Logger::get().info(__FILE__,
                                     __LINE__,
                                     "AMQP",
                                     fmt::format("Declare: queue={0}, flags={1}, consumer_tag={2}, mcnt={3}, cc={4}",
                                                 cfg->queue.name,
                                                 cfg->queue.flags,
                                                 name, messagecount, consumercount));
           })
           .onError([this](const char *msg) {
             appendErrorMessage(std::string(msg));
             wss::Logger::get()
                 .error(__FILE__, __LINE__, "AMQP", fmt::format("Unable to declare queue: {0}", msg));
           });

    channel->bindQueue(cfg->exchange.name, cfg->queue.name, cfg->routingKey)
           .onSuccess([this]() {
             wss::Logger::get().info(__FILE__,
                                     __LINE__,
                                     "AMQP",
                                     fmt::format("Bind: queue={0}, exchange={1}, routing_key={2}",
                                                 cfg->queue.name, cfg->exchange.name, cfg->routingKey));

             setValidState(true);
           })
           .onError([this](const char *err) {
             appendErrorMessage(std::string(err));
             wss::Logger::get()
                 .error(__FILE__, __LINE__, "AMQP", fmt::format("Unable to bind queue: {0}", err));
           });

    publishService = new boost::thread([this]() {
      // run the handler
      // a t the moment, one will need SIGINT to stop.  In time, should add signal handling through boost API.
      service.run();
    });
}

void wss::event::AMQPTarget::stop() {
    connection->close();
    service.stop();
    publishService->join();
    delete publishService;
    setValidState(false);
}

// now, only sequantially
// @TODO make parallel send, probably batched
void wss::event::AMQPTarget::send(const wss::MessagePayload &payload,
                                  const wss::event::Target::OnSendSuccess &successCallback,
                                  const wss::event::Target::OnSendError &errorCallback) {

    if (!isValid()) {
        errorCallback("AMQP is in error state");
        return;
    }

    m_sendLock.lock();

    try {
        channel->startTransaction();
        const std::string pl = payload.toJson();
        const char *message = pl.c_str();
        channel->publish(
            cfg->exchange.name,
            cfg->routingKey,
            message,
            cfg->getMessageFlags()
        );

        channel->commitTransaction()
               .onError([this, &errorCallback](const char *msg) {
                 errorCallback(std::string(msg));
                 m_sendLock.unlock();
               })
               .onSuccess([this, &successCallback]() {
                 successCallback();
                 m_sendLock.unlock();
               });

    } catch (const std::exception &e) {
        errorCallback(e.what());
        m_sendLock.unlock();
    }
}

std::string wss::event::AMQPTarget::getType() {
    return "amqp";
}

wss::event::Target *wss::event::target_create(const nlohmann::json &config) {
    return new AMQPTarget(config);
}

void wss::event::target_release(wss::event::Target *target) {
    delete target;
}
