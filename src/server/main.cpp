#include <iostream>
#include <unordered_map>
#include <regex>
#include "chat/ChatServer.h"
#include "../client/QueryBuilder.hpp"
#include "../client/HttpClient.h"
#include <boost/asio.hpp>

#include <boost/network/protocol/http/client.hpp>

using std::cout;
using std::cerr;
using std::endl;

using namespace boost::asio;
using namespace boost;

int main(int argc, char **argv) {

    wss::client::QueryBuilder q("google.com", "/", true);

//    wss::client::HttpClient client;
//    wss::client::Response resp;
//    q.buildOStream(&std::cout);
//    try {
//        resp = client.call(q);
//    } catch(const boost::system::system_error &ex) {
//        cerr << "Exception: " << ex.what() << endl;
//        return 1;
//    }
//
//
//    cout << "Status Code: " << resp.status << endl;
//    cout << "Status message: " << resp.statusMessage << endl;
//    cout << "Response data:" << endl << resp.data;








//    cmdline::parser args;
//    args.add<std::string>("config", 'C', "Use: wsserver -C [--config] /path/to/wsserver.ini", true);
//    args.parse(argc, argv);
//    args.parse_check(argc, argv);
//    const std::string configFile = args.get<std::string>("config");
//
//    INIConfig::Parser config(configFile);
//    if(!config.hasSection("server")) {
//        std::cerr << "Invalid config. Section [server] required" << std::endl;
//        return 1;
//    }
//    INIConfig::Section *serverConfig = config.getSection("server");
//    INIConfig::Section *restApiConfig = config.getSection("restapi");
//    INIConfig::Section *chatConfig = config.getSection("chat");
//    INIConfig::Section *eventConfig = config.getSection("event");
//
//    unsigned short port = static_cast<unsigned short>(serverConfig->getRow("port", 9001)->getValue().getInt());
//    std::string address = serverConfig->getRow("address", "*")->getValue().get();
//    bool secure = serverConfig->getRow("secure.enabled", false)->getValue().getBool();
//    std::string endpoint = serverConfig->getRow("endpoint", "/chat/")->getValue().get();
//    bool enableApi = restApiConfig->getRow("enabled", false)->getValue().getBool();
//
//    wss::ChatServer webSocket(address, port, "^" + endpoint + "?$");
//
//    if(!chatConfig->getRow("message.maxSize", "10M")->getValue().get().empty()) {
//        std::smatch res = toolboxpp::strings::matchRegexp(R"(^(\d+)(M|K)$)", chatConfig->getRow("message.maxSize", "10M")->getValue().get());
//        if(res.size() != 3) {
//            cerr << "Invalid message.maxSize value format. Must be: 10M or 1000K which means 10 megabytes or 1000 kilobytes" << endl;
//            return 1;
//        }
//
//        unsigned long sz;
//        try{
//            sz = std::stoul(res[1]);
//        }catch (...) {
//            cerr << "Invalid value message.maxSize" << endl;
//            return 1;
//        }
//
//        std::size_t maxBytes = 0;
//
//        if(res[2] == "M") {
//            // megabytes
//            maxBytes = sz * 1024 * 1024;
//        } else {
//            // kilobytes
//            maxBytes = sz * 1024;
//        }
//        webSocket.setMessageSizeLimit(maxBytes);
//    }
//
//    if (secure) {
//        webSocket.enableTLS(
//            serverConfig->getRow("secure.cert.crtPath", "")->getValue().get(),
//            serverConfig->getRow("secure.cert.keyPath", "")->getValue().get()
//        );
//    }
//
//    webSocket.setThreadPoolSize(static_cast<size_t>(
//                                    config.getSection("server")
//                                          ->getRow("workers", std::thread::hardware_concurrency()*2)
//                                          ->getValue()
//                                          .getInt()
//                                ));
//
//
//    // run ws server
//    webSocket.run();
//    if (enableApi) {
//        wss::ChatRestApi restApi("*", 8081);
//        restApi.attachToChat(&webSocket);
//        // run rest server
//        restApi.run();
//        restApi.waitThread();
//    } else {
//        webSocket.waitThread();
//    }

    return 0;
}