#include <iostream>
#include <unordered_map>
#include <regex>
#include "chat/ChatMessageServer.h"
#include "cmdline.hpp"
#include "restapi/ChatRestServer.h"
#include "event/EventNotifier.h"
#include "ServerStarter.h"

using std::cout;
using std::cerr;
using std::endl;

using json = nlohmann::json;


int main(int argc, char **argv) {

    wss::ServerStarter server(argc, argv);

    if (!server.isValid()) {
        return 1;
    }

    server.run();

    return 0;
}