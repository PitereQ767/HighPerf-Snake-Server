#pragma once

#include <SFML/Graphics.hpp>
#include "NetworkClient.hpp"

#include <SFML/Graphics/Font.hpp>


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
    void renderMenu();
    void renderHUD();

    void drawFrameArena();
    void drawSnakes();
    void drawApples();
    void drawLeaderBoard();
    void showStatistics();

    sf::RenderWindow window;
    sf::Clock clock;

    NetworkClient network;

    char ipBuffer[256]{"127.0.0.1"};
    int portBuffer{8080};

    const float TILE_SIZE = 20.0f;
    static constexpr int16_t MAP_WIDTH = 40;
    static constexpr int16_t MAP_HEIGHT = 30;

    sf::Font font;
    bool fontLoaded{false};
};