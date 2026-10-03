#include "api/Server.h"
#include <cstdlib>

int main() {
    const char* portEnv = std::getenv("PORT");
    int port = portEnv ? std::atoi(portEnv) : 8080;

    Server server;
    server.run(port);

    return 0;
}
