#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
#include <boost/thread/thread.hpp>
#include <boost/asio/io_service.hpp>
#include <unordered_set>
#include <csignal>
#include <toolboxpp.h>

#include "client_ws.hpp"
#include "json.hpp"

using namespace std::placeholders;
using namespace boost;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;
using WsConnectionPtr = std::shared_ptr<WsClient::Connection>;
using logger = toolboxpp::Logger;
using std::cout;
using std::cerr;
using std::endl;

int lastSender;
int lastRecipient;
std::atomic_int connected;

// CONFIG
const int RUN_TIMES = 5;
const int CONCURRENCY = 100;
const int MESSAGES = 100;
const boost::int_least64_t SLEEP_MS = 100;

std::recursive_mutex lock;
std::unordered_map<int, bool> conns;

struct stats_t {
  std::vector<double> sendTimes;
  std::atomic_int sentMessages;
  std::atomic_int errorMessages;
  std::atomic_int totalConnections;
  std::atomic_long payloadTransferredBytes;
  double timeTaken = 0;

  std::vector<int> calcTime(long ms) {
      int seconds = (int) (ms / 1000) % 60;
      int minutes = (int) ((ms / (1000 * 60)) % 60);
      int hours = (int) ((ms / (1000 * 60 * 60)) % 24);

      return {
          hours, minutes, seconds
      };
  }

  void calculate() {
      auto *minMax = new double[3];
      calcMinMaxTime(minMax);
      double totalSendTimes = 0.0;
      for (auto t: sendTimes) {
          totalSendTimes += t;
      }
      double transferSpeedKbps = ((payloadTransferredBytes * 8) / totalSendTimes) / 1000;

      const std::vector<int> times = calcTime(static_cast<long>(totalSendTimes));
      cout
          << endl
          << "         MIN send time: " << (minMax[0]) << "ms" << endl
          << "         MAX send time: " << (minMax[1]) << "ms" << endl
          << "         AVG send time: " << (minMax[2]) << "ms" << endl
          << "            Total sent: " << sendTimes.size() << endl
          << "         Sent messages: " << sentMessages << endl
          << "        Error messages: " << errorMessages << endl
          << "     Total connections: " << totalConnections << endl
          << "   Transferred payload: " << payloadTransferredBytes << " bytes (" << (payloadTransferredBytes / 1000)
          << " Kb)" << endl
          << "Average transfer speed: " << transferSpeedKbps << " Kb/s (" << (transferSpeedKbps / 1000) << " Mb/s)"
          << endl
          << "Benchmark time (total): " << totalSendTimes << "ms (" << times[1] << " minutes" << " " << times[2]
          << " seconds)" << endl
          << endl;

      delete[] minMax;
  }

  void calcMinMaxTime(double *result) {
      double min = 0, max = 0;
      double sum = 0.0;
      for (auto t: sendTimes) {
          sum += t;
          if (t > max) {
              max = t;
          }
      }
      min = max;
      for (auto t: sendTimes) {
          if (t < min) {
              min = t;
          }
      }

      result[0] = min;
      result[1] = max;
      result[2] = sum / static_cast<double>(sendTimes.size());

  }
}; // stats

stats_t _statistics;
std::mutex statLock;

void handleOne(int sen, int rec, WsConnectionPtr &conn) {
    std::string msg;
    nlohmann::json obj;
    std::vector<int> recps = {rec};
    obj["sender"] = sen;
    obj["recipients"] = recps;
    obj["type"] = "text";
    obj["text"] = "Hello dummy benchmarking";
    msg = obj.dump();

    auto send_stream = std::make_shared<WsClient::SendStream>();
    *send_stream << msg;

    for (int i = 0; i < MESSAGES; i++) {
        std::stringstream ss;
        ss << "client_" << sen << "_" << i;
        std::string tag = ss.str();

        toolboxpp::Profiler::get().begin(tag);
        conn->send(send_stream, [tag](const SimpleWeb::error_code &errorCode) {
          if (errorCode) {
              // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
              L_ERR_F("Send message", "Server: Error sending message. %s, error message: %s",
                      errorCode.category().name(),
                      errorCode.message().c_str()
              );
              _statistics.errorMessages++;
              toolboxpp::Profiler::get().end(tag);
              return;
          }
          double sendTime;
          toolboxpp::Profiler::get().end(tag, &sendTime);
          statLock.lock();
          _statistics.sendTimes.push_back(sendTime);
          statLock.unlock();
          _statistics.sentMessages++;
        });

        _statistics.payloadTransferredBytes += msg.length();
        boost::this_thread::sleep_for(boost::chrono::milliseconds(SLEEP_MS));
    }

    L_DEBUG_F("Client", "[%d] sent message to client [%d]: %s", sen, rec, msg.c_str());
    conn->send_close(1000, "OK");
}

void connect(int sen, int rec) {
    lock.lock();
    if (conns.count(sen)) {
        lock.unlock();
        return;
    }
    lock.unlock();
    L_DEBUG_F("Client", "[%d] Connecting...", sen);

    WsClient client(std::string("localhost:8080/chat?id=") + std::to_string(sen));
    client.on_message = [sen](WsConnectionPtr connection, std::shared_ptr<WsClient::Message> message) {
      auto message_str = message->string();

      L_DEBUG_F("Client", "[%d] Message received: %s", sen, message_str.c_str());
    };

    client.on_open = [sen, rec](WsConnectionPtr connection) {
      L_DEBUG_F("Client", "[%d] Connected!", sen);
      connected++;
      lock.lock();
      conns[sen] = true;
      lock.unlock();
      handleOne(sen, rec, connection);
    };

    client.on_close = [sen](WsConnectionPtr /*connection*/, int status, const std::string & /*reason*/) {
      L_DEBUG_F("Client", "[%d] Disconnected with status code %d", sen, status);
      lock.lock();
      conns.erase(sen);
      lock.unlock();
      connected--;
    };

    // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
    client.on_error = [sen](WsConnectionPtr /*connection*/, const SimpleWeb::error_code &ec) {
      L_ERR_F("Client", "[%d] Error: %s, error message: %s", sen, ec.category().name(), ec.message().c_str());
      lock.lock();
      conns.erase(sen);
      lock.unlock();
      connected--;
    };

    client.start();
}

void connectAll(asio::io_service *service) {
    for (int i = 2; i < CONCURRENCY + 2; i++) {
        _statistics.totalConnections++;
        service->post(boost::bind(connect, i, i));

        lastSender++;
        lastRecipient++;
    }
}

void watchdog(asio::io_service &service) {
    while (connected > 0) {
        boost::this_thread::sleep_for(boost::chrono::seconds(1));
        L_DEBUG_F("Watchdog", "Still have %d connections", connected.load())
    }

    boost::this_thread::sleep_for(boost::chrono::seconds(2));
    service.stop();
}

void run() {
    lastSender = 2;
    lastRecipient = 1;

    asio::io_service ioService;
    boost::thread_group threadGroup;
    asio::io_service::work work(ioService);

    for (int i = 0; i < CONCURRENCY; i++) {
        threadGroup.create_thread(
            boost::bind(&asio::io_service::run, &ioService)
        );
    }

    connectAll(&ioService);

    ioService.post(boost::bind(watchdog, boost::ref(ioService)));
    threadGroup.join_all();
}

int main() {
    L_LEVEL(logger::LEVEL_INFO | logger::LEVEL_ERROR | logger::LEVEL_WARNING);

    for (int i = 0; i < RUN_TIMES; i++) {
        L_INFO_F("main", "Run benchmark %d time", i + 1);
        run();
    }
    _statistics.calculate();

    return 0;
}