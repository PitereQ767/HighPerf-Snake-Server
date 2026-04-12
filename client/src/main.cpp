#include "GameClient.hpp"
#include <iostream>

int main() {
    try {
        GameClient app;
        app.run();
    }catch (const std::exception& e) {
        std::cerr << "Critical error during starting app: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
