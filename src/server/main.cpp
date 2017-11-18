#include <iostream>
#include "chat/ChatServer.h"
#include "restapi/ChatRestApi.h"

using namespace std;
using json = nlohmann::json;

int main() {

    wss::ChatServer webSocket("*", 8080, "^/chat/?$");
//    wss::ChatRestApi restApi("*", 8081);
//    restApi.attachToChat(&webSocket);
//
//
//    // run ws server
    webSocket.run();
    webSocket.waitThread();
//
//    // run rest server
//    restApi.run();
//    restApi.waitThread();




    return 0;
}