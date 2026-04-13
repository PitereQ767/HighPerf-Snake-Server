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
                    dirY = Protocol::Direction::UP; dirX = Protocol::Direction::NEUTRAL; directionChanged = true;
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

void GameClient::renderUI() {
    ImGui::Begin("Panel sterowania - Snake");

    if (!network.isConnected()) {
        ImGui::Text("Status: Disconnected");
        ImGui::Separator();
        ImGui::InputText("Adres IP", ipBuffer, sizeof(ipBuffer));
        ImGui::InputInt("Port", &portBuffer);

        if (ImGui::Button("Connect to server", ImVec2(150, 20))) {
            network.connectToServer(ipBuffer, portBuffer);
        }
    }else {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Status: Connected");
        ImGui::Separator();

        if (ImGui::Button("Disconnect", ImVec2(150, 30))) {
            network.disconnectFromServer();
        }
    }

    ImGui::End();
}

void GameClient::render() {
    window.clear(sf::Color(30, 30, 30));

    ImGui::SFML::Render(window);
    window.display();
}
