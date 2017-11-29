#include <iostream>
#include <unordered_map>
#include <regex>
#include "chat/ChatMessageServer.h"
#include "restapi/ChatRestServer.h"
#include "event/EventNotifier.h"
#include "ServerStarter.h"

#include "Settings.hpp"
using std::cout;
using std::cerr;
using std::endl;

using json = nlohmann::json;


int main(int argc, char **argv) {

    const auto **args = const_cast<const char **>(argv);
    wss::ServerStarter server(argc, args);

    if (!server.isValid()) {
        return 1;
    }

    server.run();

    return 0;
}