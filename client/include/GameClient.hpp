#pragma once

#include <SFML/Graphics.hpp>
#include "NetworkClient.hpp"

#define UP -1
#define DOWN 1
#define LEFT -1
#define RIGHT 1
#define NEUTRAL 0

class GameClient {
public:
    GameClient();
    ~GameClient();

    void run();

private:
    void processEvents();
    void update();
    void render();

    void renderUI();

    sf::RenderWindow window;
    sf::Clock clock;

    NetworkClient network;

    char ipBuffer[256]{"127.0.0.1"};
    int portBuffer{8080};
};