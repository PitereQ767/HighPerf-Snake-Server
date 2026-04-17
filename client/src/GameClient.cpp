#include "GameClient.hpp"
#include "imgui.h"
#include "imgui-SFML.h"
#include <iostream>
#include <sys/socket.h>

#include "Protocol.hpp"

GameClient::GameClient() {
    std::cout << "Initialize graphic engine..." << std::endl;

    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    window.create(sf::VideoMode(1024, 768), "Snake Client", sf::Style::Default, settings);
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Failed to initialize ImGui-SFML!" << std::endl;
    }

    if (font.loadFromFile(ASSETS_PATH "Carlito-Bold.ttf")) {
        fontLoaded = true;
    }else {
        std::cerr << "Failed to load font!" << std::endl;
    }
}

GameClient::~GameClient() {
    ImGui::SFML::Shutdown();
}

void GameClient::run() {
    while (window.isOpen()) {
        processEvents();
        update();
        render();
    }
}

void GameClient::processEvents() {
    sf::Event event{};
    while (window.pollEvent(event)) {
        ImGui::SFML::ProcessEvent(window, event);

        if (event.type == sf::Event::Closed) {
            window.close();
        }

        if (event.type == sf::Event::KeyPressed) {
            if (ImGui::GetIO().WantCaptureKeyboard) continue; //Zapobieganie aby nie sterowac wezem podczas gdy klient bedzie wpisywal adres IP.

            Protocol::Direction dirX;
            Protocol::Direction dirY;
            bool directionChanged{false};

            switch (event.key.code) {
                case sf::Keyboard::Up:
                case sf::Keyboard::W:
                    dirX = Protocol::Direction::NEUTRAL; dirY = Protocol::Direction::UP; directionChanged = true;
                    break;
                case sf::Keyboard::Right:
                case sf::Keyboard::D:
                    dirX = Protocol::Direction::RIGHT; dirY = Protocol::Direction::NEUTRAL; directionChanged = true;
                    break;
                case sf::Keyboard::Left:
                case sf::Keyboard::A:
                    dirX = Protocol::Direction::LEFT; dirY = Protocol::Direction::NEUTRAL; directionChanged = true;
                    break;
                case sf::Keyboard::Down:
                case sf::Keyboard::S:
                    dirX = Protocol::Direction::NEUTRAL; dirY = Protocol::Direction::DOWN; directionChanged = true;
                    break;
                default:
                    break;
            }

            if (directionChanged && network.isConnected()) {
                network.sendMoveDirection(dirX, dirY);
            }
        }
    }
}

void GameClient::update() {
    network.reciveData();
    ImGui::SFML::Update(window, clock.restart());
    renderUI();
}

void GameClient::renderMenu() {
    sf::Vector2u windowSize = window.getSize();

    ImGui::SetNextWindowPos(ImVec2(windowSize.x / 2.0f, windowSize.y / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::SetNextWindowSize(ImVec2(300, 200));

    ImGui::Begin("Snake login", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    static char nickBuffer[32] = "Player";
    ImGui::InputText("Nick", nickBuffer, sizeof(nickBuffer));

    ImGui::Separator();

    static float color[3] = { 0.0f, 1.0f, 0.0f };
    ImGui::ColorEdit3("Snake color", color);

    ImGui::Spacing();

    if (ImGui::Button("CONNECT & PLAY", ImVec2(-1, 40))) {

        network.connectToServer(ipBuffer, portBuffer, nickBuffer, color);
    }

    ImGui::End();
}

void GameClient::renderUI() {
    if (!network.isConnected()) {
        renderMenu();
    }else {
        renderHUD();
    }
}

void GameClient::drawFrameArena() {
    sf::RectangleShape arena(sf::Vector2f(MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE));
    arena.setPosition(0.0f, 0.0f);
    arena.setFillColor(sf::Color(30, 30, 30));
    arena.setOutlineThickness(-2.0f);
    arena.setOutlineColor(sf::Color::Red);
    window.draw(arena);
}

void GameClient::drawSnakes() {
    const auto& players = network.getPlayers();
    for (const auto& player : players) {
        for (const auto& segment : player.body) {
            sf::RectangleShape snakeRect(sf::Vector2f(TILE_SIZE - 1.0f, TILE_SIZE - 1.0f));

            snakeRect.setPosition(segment.x * TILE_SIZE, segment.y * TILE_SIZE);
            snakeRect.setFillColor(sf::Color(player.color.r, player.color.g, player.color.b));

            window.draw(snakeRect);
        }
    }
}

void GameClient::drawApples() {
    const auto& apples = network.getApples();
    for (const auto& apple : apples) {
        sf::RectangleShape appleRect(sf::Vector2f(TILE_SIZE - 4.0f, TILE_SIZE - 4.0f));
        appleRect.setPosition(apple.x * TILE_SIZE + 2.0f, apple.y * TILE_SIZE + 2.0f);
        appleRect.setFillColor(sf::Color::Red);
        window.draw(appleRect);
    }
}

void GameClient::render() {
    window.clear(sf::Color(30, 30, 30));

    if (network.isConnected()) {
        drawFrameArena();
        drawSnakes();
        drawApples();
        drawLeaderBoard();
    }

    ImGui::SFML::Render(window);
    window.display();
}

void GameClient::drawLeaderBoard() {
    if (fontLoaded) {
        auto players = network.getPlayers();
        std::sort(players.begin(), players.end(), [](const auto& a, const auto& b) {
            return a.score > b.score;
        });

        sf::Text uiText;
        uiText.setFont(font);
        uiText.setCharacterSize(15);

        float positionY = 10.0f;
        float positionX = MAP_WIDTH * TILE_SIZE + 10.0f;
        uiText.setFillColor(sf::Color::Yellow);
        uiText.setString("--- TOP GRACZY ---");
        uiText.setPosition(positionX, positionY);
        window.draw(uiText);

        positionY += 20.0f;
        uiText.setFillColor(sf::Color::White);
        uiText.setCharacterSize(13);
        int displayCount = std::min(5, static_cast<int>(players.size()));
        for (int i = 0; i < displayCount; ++i) {
            std::string record = std::to_string(i + 1) +
                                ". " + players[i].nick + ": "
                                + std::to_string(players[i].score);
            uiText.setString(record);
            uiText.setPosition(positionX, positionY);
            window.draw(uiText);
            positionY += 20.0f;
        }
    }else {
        std::cerr << "Font not loaded" << std::endl;
    }
}

void GameClient::renderHUD() {
    // Zakładając okno 1024x768 i mapę 800x600, dajemy to na samym dole po prawej
    sf::Vector2u windowSize = window.getSize();
    ImGui::SetNextWindowPos(ImVec2(windowSize.x - 220.0f, windowSize.y - 120.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(200.0f, 100.0f), ImGuiCond_Always);

    ImGuiWindowFlags hudFlags = ImGuiWindowFlags_NoDecoration |
                                ImGuiWindowFlags_NoMove |
                                ImGuiWindowFlags_NoSavedSettings |
                                ImGuiWindowFlags_NoBackground;

    ImGui::Begin("HUD", nullptr, hudFlags);

    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: CONNECTED");
    ImGui::Spacing();

    // Podmieniamy domyślne kolory ImGui tylko dla tego jednego przycisku
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));        // Normalny czerwony
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 1.0f)); // Jasny czerwony po najechaniu
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));  // Ciemny czerwony przy kliknięciu

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

    if (ImGui::Button("DISCONNECT", ImVec2(-1, 40))) { // -1 oznacza "wypełnij całą szerokość"
        network.disconnectFromServer();
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImGui::End();
}
