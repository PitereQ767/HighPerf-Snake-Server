#include <iostream>
#include <ostream>

#include "Server.hpp"

int main() {

    Server server(8080);
    if (!server.start()) {
        std::cerr << "Error starting server" << std::endl;
        return 1;
    }

    server.run();
    return 0;
}
