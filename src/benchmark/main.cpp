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
std::recursive_mutex lock;


// CONFIG
const int RUN_TIMES = 5;
const int CONCURRENCY = 50;
const int MESSAGES = 100;
const boost::int_least64_t SLEEP_MS = 150;

std::unordered_map<int, WsConnectionPtr> conns(CONCURRENCY);

const static std::string DUMMY_TEXT =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Phasellus egestas, dui euismod aliquet ultrices, magna ex tincidunt ante, non tincidunt enim quam at ipsum. Etiam vel ligula pulvinar, pharetra magna at, finibus eros. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Nullam nibh nibh, tincidunt sit amet posuere eget, lobortis eu quam. Sed tincidunt, nulla ac fringilla cursus, velit nibh condimentum nisl, eget suscipit orci ex at nibh. Nam malesuada, purus nec cursus feugiat, elit erat porta quam, in maximus dui nisi ut tortor. Duis felis magna, maximus quis tristique ac, pellentesque at elit. Pellentesque augue felis, vehicula ut egestas luctus, aliquet ut justo. Quisque iaculis ligula sit amet sollicitudin facilisis. Vivamus congue suscipit metus, vitae eleifend neque ornare nec. Nulla facilisi. Phasellus augue leo, laoreet non dapibus malesuada, auctor et purus. Etiam tempor tempus sagittis. Duis porta iaculis velit quis pretium. Donec at arcu luctus, vulputate magna id, tincidunt arcu.\n"
        "\n"
        "Integer fermentum vulputate lorem, eget aliquam lectus interdum at. In lobortis, lectus id pellentesque porttitor, magna nunc vestibulum urna, eu auctor nisl eros eget nibh. Fusce augue diam, tempus nec nisi ac, pharetra tempor erat. Aliquam ullamcorper urna vitae elit sagittis ornare. Quisque aliquam diam lorem, eget suscipit tellus venenatis ultrices. Proin at elit quis lectus accumsan mattis. Ut vel commodo justo. Vivamus gravida euismod lacus eu mattis. Duis vitae porttitor ligula, non pellentesque turpis. Donec sollicitudin lectus a sem egestas, eget lacinia libero blandit. Ut consequat nisl felis, id sodales tortor ultrices sed.\n"
        "\n"
        "Donec dictum gravida est. In hac habitasse platea dictumst. Sed in tristique est, in volutpat odio. Sed finibus felis mi, eget scelerisque orci interdum et. Proin ac orci non libero facilisis fringilla. Duis eget eleifend elit. Praesent consequat diam elit, lobortis porta tellus blandit at. Proin a felis nisl. Cras fermentum sollicitudin urna, non blandit erat scelerisque quis. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas.\n"
        "\n"
        "Maecenas elementum placerat pulvinar. Praesent porttitor maximus lorem, quis convallis magna laoreet maximus. Proin iaculis dapibus bibendum. Pellentesque ac velit feugiat, varius nulla nec, lobortis nibh. Nunc condimentum nisi ac malesuada tempor. Nunc et magna sed lectus viverra mollis id nec orci. Nunc aliquam euismod dui, eu scelerisque purus ornare quis. Suspendisse magna ipsum, porttitor ornare risus ac, aliquam lacinia quam. Donec venenatis faucibus consectetur. Integer ultrices, libero et imperdiet ullamcorper, magna elit laoreet velit, a auctor sem justo eu massa. Morbi ornare accumsan orci, a blandit massa vestibulum vitae. Vestibulum tortor velit, convallis eu tortor et, maximus rhoncus urna. Suspendisse vitae pulvinar sapien.\n"
        "\n"
        "Duis condimentum justo vitae malesuada laoreet. Pellentesque vitae lorem risus. Fusce at nulla vitae ex vestibulum viverra id ac risus. Ut non leo lectus. Phasellus venenatis erat non odio maximus sagittis. Duis id laoreet nulla. Cras lacinia eros sit amet sodales rhoncus. Fusce convallis convallis facilisis. Phasellus cursus tincidunt libero, sed placerat quam tempus quis. Nullam pellentesque, quam ut auctor tristique, diam justo tristique felis, a bibendum urna ipsum eget odio. Etiam congue euismod convallis.";

struct stats_t {
  std::vector<double> sendTimes;
  std::atomic_int sentMessages;
  std::atomic_int errorMessages;
  std::atomic_int totalConnections;
  std::atomic_long payloadTransferredBytes;
  double timeTaken = 0;

  std::vector<int> calcTime(long ms) {
      int seconds = (int) (ms / 1000) % 60;
      auto minutes = (int) ((ms / (1000 * 60)) % 60);
      auto hours = (int) ((ms / (1000 * 60 * 60)) % 24);

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
    obj["text"] = DUMMY_TEXT;
    msg = obj.dump();

    for (int i = 0; i < MESSAGES; i++) {
        std::stringstream ss;
        ss << "client_" << sen << "_" << i;
        const std::string tag = ss.str();

        // DO NOT reuse stream, it will die after first sending
        const std::shared_ptr<WsClient::SendStream> send_stream = std::make_shared<WsClient::SendStream>();
        *send_stream << msg;

        toolboxpp::Profiler::get().begin(tag);
        sendLock.lock();
        try {
            // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
            conn->send(send_stream, [tag](const SimpleWeb::error_code &errorCode) {
              if (errorCode) {
                  std::stringstream ss;
                  ss << "Server: Error sending message. " << errorCode.category().name()
                     << " error message: " << errorCode.message() << endl;

                  toolboxpp::Logger::get().debug("Send Message", ss.str());

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
              sendLock.unlock();
            });
        } catch (const std::exception &e) {
            cerr << e.what() << endl;
            sendLock.unlock();
        }

        _statistics.payloadTransferredBytes += msg.length();
        boost::this_thread::sleep_for(boost::chrono::milliseconds(SLEEP_MS));
    }

    L_DEBUG_F("Client", "[%d] sent message to client [%d]", sen, rec);
    conn->send_close(1000, "OK");
}

boost::thread_group ct;

void connect(int sen, int rec) {
    if (conns.count(sen)) {
        return;
    }

    L_DEBUG_F("Client", "[%d] Connecting...", sen);

    WsClient client(std::string("localhost:8085/chat?id=") + std::to_string(sen));
    client.on_message = [sen](WsConnectionPtr, std::shared_ptr<WsClient::Message> msg) {
//        cout << "Client: [" << sen << "] Message received: "<< endl;
    };

    client.on_open = [sen, rec](WsConnectionPtr connection) {
      L_DEBUG_F("Client", "[%d] Connected!", sen);
      conns[sen] = connection;
      connected++;
    };

    client.on_close = [sen](WsConnectionPtr /*connection*/, int status, const std::string &reason) {
      const char *s = reason.c_str();
      L_DEBUG_F("Client", "[%d] Disconnected with status code %d: %s", sen, status, s);
      conns.erase(sen);
      connected--;
    };

    // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
    client.on_error = [sen](WsConnectionPtr /*connection*/, const SimpleWeb::error_code &ec) {
      L_ERR_F("Client", "[%d] Error: %s, error message: %s", sen, ec.category().name(), ec.message().c_str());
      conns.erase(sen);
      connected--;
    };

    client.start();
}

void connectAll(asio::io_service *service) {
    for (int i = 1; i < CONCURRENCY + 1; i += 2) {
        service->post(boost::bind(&connect, i, i + 1));
        service->post(boost::bind(&connect, i + 1, i));
        _statistics.totalConnections += 2;
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
    asio::io_service ioService;
    boost::thread_group threadGroup;
    asio::io_service::work work(ioService);

    // run threads
    for (int i = 0; i < CONCURRENCY * 2; i++) {
        threadGroup.create_thread(
            boost::bind(&asio::io_service::run, &ioService)
        );
    }

    // connect
    connectAll(&ioService);


    // wait for all connected
    while (connected < CONCURRENCY) {
        cout << "Waiting for connecting all connections..." << endl;
        boost::this_thread::sleep_for(boost::chrono::seconds(1));
    }

    // run message exchanges
    for (int i = 1; i < CONCURRENCY + 1; i += 2) {
        ioService.post(boost::bind(&handleOne, i, i + 1, conns[i]));
        ioService.post(boost::bind(&handleOne, i + 1, i, conns[i + 1]));
    }

    ioService.post(boost::bind(watchdog, boost::ref(ioService)));

    ct.join_all();
    threadGroup.join_all();

    conns.clear();
}

int main() {
    L_LEVEL(logger::LEVEL_INFO | logger::LEVEL_ERROR | logger::LEVEL_WARNING);

    if (CONCURRENCY % 2 != 0) {
        cerr << "Concurrent connections number must be even" << endl;
        return 1;
    }

    for (int i = 0; i < RUN_TIMES; i++) {
        L_INFO_F("main", "Run benchmark %d time", i + 1);
        run();
    }
    _statistics.calculate();

    return 0;
}
