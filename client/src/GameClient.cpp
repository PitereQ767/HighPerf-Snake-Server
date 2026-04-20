#include "GameClient.hpp"

#include <cmath>

#include "imgui.h"
#include "imgui-SFML.h"
#include <SFML/Graphics/Color.hpp>
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
    network.updateNetworkState();

    float dt = clock.restart().asSeconds();
    ImGui::SFML::Update(window, sf::seconds(dt));

    syncVisualPlayers();

    updateVisualSnakes(dt);

    renderUI();
}

void GameClient::renderMenu() {
    sf::Vector2u windowSize = window.getSize();

    ImGui::SetNextWindowPos(ImVec2(windowSize.x / 2.0f, windowSize.y / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::SetNextWindowSize(ImVec2(300, 200));

    ImGui::Begin("Snake login", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::InputText("IP", ipBuffer, sizeof(ipBuffer));
    ImGui::InputInt("Port", &portBuffer, sizeof(portBuffer));

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
    float arenaW = MAP_WIDTH * TILE_SIZE;
    float arenaH = MAP_HEIGHT * TILE_SIZE;

    sf::RectangleShape arena(sf::Vector2f(arenaW, arenaH));
    arena.setFillColor(sf::Color(25, 25, 30));
    // arena.setOutlineThickness(3.0f);
    arena.setOutlineColor(sf::Color(200, 50, 50));
    window.draw(arena);

    sf::Color gridColor(38, 38, 44);
    for (int x = 1; x < MAP_WIDTH; ++x) {
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(x * TILE_SIZE, 0), gridColor),
            sf::Vertex(sf::Vector2f(x * TILE_SIZE, arenaH), gridColor)
        };
        window.draw(line, 2, sf::Lines);
    }
    for (int y = 1; y < MAP_HEIGHT; ++y) {
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(0, y * TILE_SIZE), gridColor),
            sf::Vertex(sf::Vector2f(arenaW, y * TILE_SIZE), gridColor)
        };
        window.draw(line, 2, sf::Lines);
    }

    // float borderThickness = 3.0f;
    // sf::RectangleShape border(sf::Vector2f(arenaW + borderThickness * 2, arenaH + borderThickness * 2));
    // border.setPosition(-borderThickness, -borderThickness);
    // border.setFillColor(sf::Color::Transparent);
    // border.setOutlineThickness(-borderThickness);
    // border.setOutlineColor(sf::Color(200, 50, 50));
    // window.draw(border);
}

void GameClient::drawSnakes() {
    const auto& players = network.getPlayers();
    for (const auto& player : players) {
        auto it = visualPlayers.find(player.playerId);
        if (it == visualPlayers.end()) continue;
        auto& vp = it->second;

        for (size_t i = 0; i < vp.visualBody.size() && i < player.body.size(); i++) {
            bool isHead = (i == 0);
            auto& vs = vp.visualBody[i];

            sf::RectangleShape snakeRect(sf::Vector2f(TILE_SIZE - 1.0f, TILE_SIZE - 1.0f));
            snakeRect.setPosition(vs.currentPos); // interpolowana pozycja

            if (isHead) {
                snakeRect.setFillColor(sf::Color(
                    std::min(player.color.r + 80, 255),
                    std::min(player.color.g + 80, 255),
                    std::min(player.color.b + 80, 255)));
                snakeRect.setOutlineThickness(-1.5f);
                snakeRect.setOutlineColor(sf::Color::White);
            } else {
                snakeRect.setFillColor(sf::Color(player.color.r, player.color.g, player.color.b));
            }
            window.draw(snakeRect);
        }

        if (!vp.visualBody.empty() && fontLoaded) {
            sf::Text nameTag;
            nameTag.setFont(font);
            nameTag.setString(player.nick);
            nameTag.setCharacterSize(10);
            nameTag.setFillColor(sf::Color::White);
            nameTag.setPosition(vp.visualBody[0].currentPos.x,
                                vp.visualBody[0].currentPos.y - 14.0f);
            window.draw(nameTag);
        }
    }
}

void GameClient::drawApples() {
    const auto& apples = network.getApples();
    for (const auto& apple : apples) {
        sf::CircleShape appleCircle(TILE_SIZE / 2.0f - 2.0f);
        appleCircle.setPosition(apple.x * TILE_SIZE + 2.0f, apple.y * TILE_SIZE + 2.0f);
        appleCircle.setFillColor(sf::Color(220, 30, 30));
        window.draw(appleCircle);
    }
}

void GameClient::render() {
    window.clear(sf::Color(30, 30, 30));

    if (network.isConnected()) {
        drawFrameArena();
        drawSnakes();
        drawApples();
        drawLeaderBoard();
        showStatistics();
    }

    ImGui::SFML::Render(window);
    window.display();
}

void GameClient::drawLeaderBoard() {
    float panelX = MAP_WIDTH * TILE_SIZE;
    float panelW = static_cast<float>(window.getSize().x) - panelX;
    float panelH = static_cast<float>(window.getSize().y);

    sf::RectangleShape sidePanel(sf::Vector2f(panelW, panelH));
    sidePanel.setPosition(panelX, 0);
    sidePanel.setFillColor(sf::Color(18, 18, 24));
    window.draw(sidePanel);

    sf::RectangleShape divider(sf::Vector2f(1.0f, panelH));
    divider.setPosition(panelX, 0);
    divider.setFillColor(sf::Color(200, 50, 50));
    window.draw(divider);

    float panelY = MAP_HEIGHT * TILE_SIZE;
    sf::RectangleShape divider2(sf::Vector2f(panelX, 1.0f));
    divider2.setPosition(0, panelY);
    divider2.setFillColor(sf::Color(200, 50, 50));
    window.draw(divider2);

    float panelDownW = window.getSize().x - panelW;
    float panelDownH = static_cast<float>(window.getSize().y) - panelY;
    sf::RectangleShape panelDown(sf::Vector2f(panelDownW, panelDownH));
    panelDown.setPosition(0, panelY + 1.0f);
    panelDown.setFillColor(sf::Color(18, 18, 24));
    window.draw(panelDown);

    if (!fontLoaded) return;

    float padX = 14.0f;
    float posX = panelX + padX;
    float posY = 16.0f;

    sf::Text title;
    title.setFont(font);
    title.setCharacterSize(17);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(sf::Color(255, 215, 0));
    title.setString("LEADERBOARD");
    title.setPosition(posX, posY);
    window.draw(title);
    posY += 28.0f;

    sf::RectangleShape titleLine(sf::Vector2f(panelW - padX * 2, 1.0f));
    titleLine.setPosition(posX, posY);
    titleLine.setFillColor(sf::Color(80, 80, 100));
    window.draw(titleLine);
    posY += 10.0f;

    auto players = network.getPlayers();
    std::sort(players.begin(), players.end(), [](const auto& a, const auto& b) {
        return a.score > b.score;
    });

    int displayCount = std::min(10, static_cast<int>(players.size()));
    float rowH = 26.0f;
    float colorBoxSize = 10.0f;

    for (int i = 0; i < displayCount; ++i) {
        const auto& p = players[i];
        float rowY = posY + i * rowH;

        if (i % 2 == 0) {
            sf::RectangleShape rowBg(sf::Vector2f(panelW - padX * 2, rowH));
            rowBg.setPosition(posX, rowY);
            rowBg.setFillColor(sf::Color(28, 28, 36));
            window.draw(rowBg);
        }

        sf::RectangleShape colorBox(sf::Vector2f(colorBoxSize, colorBoxSize));
        colorBox.setPosition(posX + 4.0f, rowY + (rowH - colorBoxSize) / 2.0f);
        colorBox.setFillColor(sf::Color(p.color.r, p.color.g, p.color.b));
        window.draw(colorBox);

        sf::Text rankText;
        rankText.setFont(font);
        rankText.setCharacterSize(13);

        sf::Color rankColor;
        if (i == 0) rankColor = sf::Color(255, 215, 0);
        else if (i == 1) rankColor = sf::Color(192, 192, 192);
        else if (i == 2) rankColor = sf::Color(205, 127, 50);
        else rankColor = sf::Color(160, 160, 170);

        rankText.setFillColor(rankColor);
        rankText.setString("#" + std::to_string(i + 1));
        rankText.setPosition(posX + 20.0f, rowY + 4.0f);
        window.draw(rankText);

        sf::Text nickText;
        nickText.setFont(font);
        nickText.setCharacterSize(13);
        nickText.setFillColor(sf::Color(220, 220, 230));
        nickText.setString(p.nick);
        nickText.setPosition(posX + 48.0f, rowY + 4.0f);
        window.draw(nickText);

        sf::Text scoreText;
        scoreText.setFont(font);
        scoreText.setCharacterSize(13);
        scoreText.setStyle(sf::Text::Bold);
        scoreText.setFillColor(sf::Color(100, 220, 100));
        scoreText.setString(std::to_string(p.score));
        float scoreWidth = scoreText.getLocalBounds().width;
        scoreText.setPosition(panelX + panelW - padX - scoreWidth - 4.0f, rowY + 4.0f);
        window.draw(scoreText);
    }

    if (players.empty()) {
        sf::Text emptyText;
        emptyText.setFont(font);
        emptyText.setCharacterSize(12);
        emptyText.setFillColor(sf::Color(100, 100, 120));
        emptyText.setString("Waiting for players...");
        emptyText.setPosition(posX, posY);
        window.draw(emptyText);
    }
}

void GameClient::renderHUD() {
    float panelX = MAP_WIDTH * TILE_SIZE + 1.0f;
    float panelW = static_cast<float>(window.getSize().x) - panelX;
    float panelH = static_cast<float>(window.getSize().y);
    float padX = 14.0f;
    float hudHeight = 90.0f;
    float hudY = panelH - hudHeight;

    ImGui::SetNextWindowPos(ImVec2(panelX, hudY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelW, hudHeight), ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padX, 10.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.07f, 0.09f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    ImGuiWindowFlags hudFlags = ImGuiWindowFlags_NoDecoration |
                                ImGuiWindowFlags_NoMove |
                                ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("HUD", nullptr, hudFlags);

    int ping = network.getPing();
    ImVec4 statusColor = ping < 50 ? ImVec4(0.3f, 0.85f, 0.3f, 1.0f)
                        : ping < 120 ? ImVec4(0.85f, 0.8f, 0.2f, 1.0f)
                        : ImVec4(0.85f, 0.2f, 0.2f, 1.0f);

    ImGui::TextColored(statusColor, ">> ONLINE");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.55f, 1.0f), "  |  %d ms", ping);
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.08f, 0.08f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

    if (ImGui::Button("DISCONNECT", ImVec2(-1, 32))) {
        network.disconnectFromServer();
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
}

void GameClient::showStatistics() {
    if (!fontLoaded) return;

    float panelX = 0.0f;
    float panelY = MAP_HEIGHT * TILE_SIZE + 1.0f;
    float panelW = MAP_WIDTH * TILE_SIZE;
    float panelH = static_cast<float>(window.getSize().y) - panelY;
    float padX = 14.0f;
    float posX = panelX + padX;
    float posY = panelY + 12.0f;

    sf::Text title;
    title.setFont(font);
    title.setCharacterSize(15);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(sf::Color(140, 170, 220));
    title.setString("NETWORK STATS");
    title.setPosition(posX, posY);
    window.draw(title);
    posY += 22.0f;

    sf::RectangleShape titleLine(sf::Vector2f(panelW - padX * 2, 1.0f));
    titleLine.setPosition(posX, posY);
    titleLine.setFillColor(sf::Color(60, 60, 80));
    window.draw(titleLine);
    posY += 12.0f;

    float colWidth = (panelW - padX * 2) / 4.0f;
    float dotRadius = 4.0f;

    struct StatEntry {
        std::string label;
        std::string value;
        sf::Color color;
    };

    float fps = ImGui::GetIO().Framerate;
    int ping = network.getPing();
    int packets = network.getPacketsPerSecond();
    float kbps = network.getLastBytePerSecond() / 1024.0f;

    char kbpsBuf[16];
    snprintf(kbpsBuf, sizeof(kbpsBuf), "%.1f", kbps);

    sf::Color fpsColor = fps >= 55 ? sf::Color(100, 220, 100) : (fps >= 30 ? sf::Color(220, 200, 50) : sf::Color(220, 50, 50));
    sf::Color pingColor = ping < 50 ? sf::Color(100, 220, 100) : (ping < 120 ? sf::Color(220, 200, 50) : sf::Color(220, 50, 50));

    StatEntry stats[] = {
        {"FPS",       std::to_string(static_cast<int>(fps)),  fpsColor},
        {"PING",      std::to_string(ping) + " ms",           pingColor},
        {"RECV",      std::to_string(packets) + " pkt/s",     sf::Color(180, 180, 200)},
        {"BANDWIDTH", std::string(kbpsBuf) + " KB/s",         sf::Color(180, 180, 200)},
    };

    for (int i = 0; i < 4; ++i) {
        float colX = posX + i * colWidth;

        sf::CircleShape dot(dotRadius);
        dot.setFillColor(stats[i].color);
        dot.setPosition(colX, posY + 2.0f);
        window.draw(dot);

        sf::Text label;
        label.setFont(font);
        label.setCharacterSize(10);
        label.setFillColor(sf::Color(120, 120, 140));
        label.setString(stats[i].label);
        label.setPosition(colX + dotRadius * 2 + 6.0f, posY - 1.0f);
        window.draw(label);

        sf::Text value;
        value.setFont(font);
        value.setCharacterSize(14);
        value.setStyle(sf::Text::Bold);
        value.setFillColor(stats[i].color);
        value.setString(stats[i].value);
        value.setPosition(colX + dotRadius * 2 + 6.0f, posY + 12.0f);
        window.draw(value);
    }
}

void GameClient::syncVisualPlayers() {
    const auto& players = network.getPlayers();

    for (const auto& player : players) {
        auto& vp = visualPlayers[player.playerId];

        if (vp.visualBody.empty()) {
            initializeVisualPlayer(vp, player);
            continue;
        }

        while (vp.visualBody.size() < player.body.size()) {
            size_t idx = vp.visualBody.size();
            VisualSegment newSeg;
            newSeg.currentPos = {player.body[idx].x * TILE_SIZE,
                                 player.body[idx].y * TILE_SIZE};
            newSeg.lastTargetPos = newSeg.currentPos;
            vp.visualBody.push_back(newSeg);
        }
        while (vp.visualBody.size() > player.body.size()) {
            vp.visualBody.pop_back();
        }

        for (int i = 0; i < player.body.size(); ++i) {
            sf::Vector2f newTarget(player.body[i].x * TILE_SIZE, player.body[i].y * TILE_SIZE);

            auto& vs = vp.visualBody[i];

            if (vs.lastTargetPos == newTarget) {
                continue;
            }

            float dx = newTarget.x - vs.lastTargetPos.x;
            float dy = newTarget.y - vs.lastTargetPos.y;

            if (dx != 0.f && dy != 0.f) {
                sf::Vector2f intermediate(newTarget.x, vs.lastTargetPos.y);
                vs.pathQueue.push(intermediate);
            }

            vs.pathQueue.push(newTarget);
            vs.lastTargetPos = newTarget;
        }
    }
}

void GameClient::initializeVisualPlayer(VisualPlayer &vp, const ClientPlayer &player) {
    vp.visualBody.resize(player.body.size());
    for (size_t i = 0; i < player.body.size(); ++i) {
        VisualSegment& vs = vp.visualBody[i];
        sf::Vector2f gridPixel(player.body[i].x * TILE_SIZE, player.body[i].y * TILE_SIZE);

        vs.currentPos = gridPixel;
        vs.lastTargetPos = gridPixel;

        while (!vs.pathQueue.empty()) vs.pathQueue.pop();
    }
}

void GameClient::updateVisualSnakes(float dt) {
    float serverTick = 0.1f;
    float spped = TILE_SIZE / serverTick;

    for (auto& [id, vp] : visualPlayers) {
        for (auto& vs : vp.visualBody) {
            float pixeles = spped * dt;

            while (pixeles > 0.f && !vs.pathQueue.empty()) {
                sf::Vector2f target = vs.pathQueue.front();
                sf::Vector2f diff = target - vs.currentPos;
                float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

                if (dist <= pixeles) {
                    vs.currentPos = target;
                    vs.pathQueue.pop();
                    pixeles -= dist;
                }else {
                    sf::Vector2f direction = diff / dist;
                    vs.currentPos += direction * pixeles;
                    pixeles = 0.f;
                }
            }
        }
    }
}
