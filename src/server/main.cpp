#include <iostream>
#include "base/ServerStarter.h"

int main(int argc, char **argv) {

    const auto **args = const_cast<const char **>(argv);
    wss::ServerStarter server(argc, args);

    if (!server.isValid()) {
        return 1;
    }

    server.run();

    return 0;
}