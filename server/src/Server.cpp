#include "Server.hpp"

#include <cstring>
#include <iostream>
#include <ostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/timerfd.h>
#include <random>

#include "Protocol.hpp"

Server::Server(int port) : port(port), events(64) {
    std::cout << "Konstrukyotr serwera (Port: " << port << ")" << std::endl;
}

Server::~Server() {
    std::cout << "Destruktor serwera. Zamykanie gniazd..." << std::endl;

    if (serverFd != -1) {
        close(serverFd);
    }

    if (epollFd != -1) {
        close(epollFd);
    }
}

bool Server::start() {
    if (!setUpServerSocket()) {
        return false;
    }

    epollFd = epoll_create1(0);
    if (epollFd == -1) {
        std::cerr << "Error creating epoll" << std::endl;
        return false;
    }

    epoll_event event{};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = serverFd;

    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &event) == -1) {
        std::cerr << "Error adding EPOLL_CTL_ADD (serverFd)" << std::endl;
        return false;
    }

    if (!setUpTimer()) {
        return false;
    }

    for (int i = 0; i < 5; i++) {
        spawnApple();
    }

    isRunning = true;
    std::cout << "Initialize server successful." << std::endl;
    return true;
}

void Server::run() {
    std::cout << "Start main event loop" << std::endl;

    while (isRunning) {
        int numEvents = epoll_wait(epollFd, events.data(), events.size(), -1);

        for (size_t i = 0; i < numEvents; i++) {
            int activeFd = events[i].data.fd;
            if (activeFd == serverFd) {
                handleNewConnection();
            }else if (activeFd == timerFd) {
                uint64_t expirations;
                read(timerFd, &expirations, sizeof(uint64_t));

                updateGameState();
            }else {
                handleClientData(activeFd);
            }
        }
    }
}

bool Server::setUpServerSocket() {
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1) {
        std::cerr << "Error creating socket (serverFd)" << std::endl;
        return false;
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverFd, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == -1) {
        std::cerr << "Error binding socket (serverFd)" << std::endl;
        return false;
    }

    if (listen(serverFd, SOMAXCONN) == -1) {
        std::cerr << "Error Listen" << std::endl;
        return false;
    }

    setNonBlocking(serverFd);
    return true;
}

bool Server::setNonBlocking(int activeFd) {
    int flags = fcntl(activeFd, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(activeFd, F_SETFL, flags | O_NONBLOCK) != -1;
}

void Server::handleNewConnection() {
    sockaddr_in clientAddr{};
    socklen_t clientAddrLen = sizeof(clientAddr);

    while (true) {
        int clientFd = accept(serverFd, reinterpret_cast<sockaddr *>(&clientAddr), &clientAddrLen);
        if (clientFd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }else {
                std::cerr << "Error accepting connection (serverFd)" << std::endl;
                break;
            }
        }

        setNonBlocking(clientFd);

        epoll_event event{};
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = clientFd;
        epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &event);

        std::cout << "New connection: " << clientFd <<  " IP: " << inet_ntoa(clientAddr.sin_addr) << std::endl;
    }
}

void Server::disconnectClient(int clientFd) {
    auto it = players.find(clientFd);
    if (it != players.end()) {
        players.erase(it);
    }else {
        std::cout << "Undefined player disconected. FD: " << clientFd << std::endl;
    }

    close(clientFd);
}

void Server::handleClientData(int clientFd) {
    alignas(alignof(Protocol::MovePacket)) uint8_t buffer[512];

    while (true) {
        ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);

        if (bytesRead <= 0) {
            if (bytesRead == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
                std::cout << "Player " << clientFd << " disconnected" << std::endl;
                disconnectClient(clientFd);
            }
            break;
        }else {
            std::cout << "Odebrano: " << bytesRead << " bajtow." << std::endl;
            processingData(buffer, clientFd, bytesRead);
        }
    }
}

void Server::processingData(uint8_t *buffer, int clientFd, ssize_t bytesRead) {
    auto messageType = static_cast<Protocol::MessageType>(buffer[0]);

    switch (messageType) {
        case Protocol::MessageType::JOIN_REQUEST: {
            uint16_t newId = nextPlayerId++;
            auto newPlayer = std::make_shared<Player>(newId, clientFd);

            newPlayer->dirX = Protocol::Direction::RIGHT;
            newPlayer->dirY = Protocol::Direction::NEUTRAL;

            newPlayer->body.push_back({10, 10});
            newPlayer->body.push_back({9, 10});
            newPlayer->body.push_back({8, 10});

            players[clientFd] = newPlayer;
            std::cout << "Player " << clientFd << " joined" << std::endl;
            break;
        }
        case Protocol::MessageType::PLAYER_MOVE:{
            if (bytesRead >= sizeof(Protocol::MovePacket)) {
                auto moveData = reinterpret_cast<Protocol::MovePacket *>(buffer);

                auto whichPlayer = players.find(clientFd);
                if (whichPlayer != players.end()) {
                    auto player = whichPlayer->second;

                    if ((player->dirX == Protocol::Direction::NEUTRAL && moveData->dirX != Protocol::Direction::NEUTRAL)
                        || player->dirY == Protocol::Direction::NEUTRAL && moveData->dirY != Protocol::Direction::NEUTRAL) {

                        player->dirX = moveData->dirX;
                        player->dirY = moveData->dirY;
                    }

                    std::cout << "Player " << clientFd << " turns -> Vector: " << (int)player->dirX << ", " << (int)player->dirY << std::endl;
                }
            }else {
                std::cerr << "Ignored truncated packet" << std::endl;
            }
            break;
        }
    }
}

bool Server::setUpTimer() {
    timerFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (timerFd == -1) {
        std::cerr << "Error creating timerfd" << std::endl;
        return false;
    }

    itimerspec timerConfig{};
    timerConfig.it_value.tv_sec = 0;
    timerConfig.it_value.tv_nsec = 100 * 1000000;
    timerConfig.it_interval.tv_sec = 0;
    timerConfig.it_interval.tv_nsec = 100 * 1000000;

    timerfd_settime(timerFd, 0, &timerConfig, nullptr);

    epoll_event event{};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = timerFd;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, timerFd, &event);

    std::cout << "Clock is running with frequency 10 TPS." << std::endl;
    return true;
}

void Server::updateGameState() {
    moveSnakes();

    if (players.empty()) return;

    uint8_t buffer[1024];
    size_t offset = 0;
    buildGameStatePacket(buffer, offset);
    broadcastGameStatePacket(buffer, offset);
}

void Server::moveSnakes() {
    for (auto& [fd, player] : players) {
        if (player->dirX != Protocol::Direction::NEUTRAL || player->dirY != Protocol::Direction::NEUTRAL) {
           Protocol::SnakeSegment head = player->body.front();

            Protocol::SnakeSegment newHead;
            newHead.x = head.x + static_cast<int16_t>(player->dirX);
            newHead.y = head.y + static_cast<int16_t>(player->dirY);

            player->body.push_front(newHead);

            if (!ateApple(newHead, *player)) {
                player->body.pop_back();
            }

            //do zaimplemontowania kolizje i jablka
        }
    }
}

bool Server::ateApple(Protocol::SnakeSegment& head, Player& player) {
    for (auto it = apples.begin(); it != apples.end(); ++it) {
        if (it->x == head.x && it->y == head.y) {
            player.score++;
            apples.erase(it);
            spawnApple();
            std::cout << "Gracz zjadl jablko. Wynik: " << player.score << std::endl;
            return true;
        }
    }

    return false;
}

void Server::buildGameStatePacket(uint8_t* buffer, size_t& offset) {
    buffer[offset] = static_cast<uint8_t>(Protocol::MessageType::GAME_STATE);
    offset++;

    uint16_t numApples = static_cast<uint16_t>(apples.size());
    memcpy(buffer + offset, &numApples, sizeof(numApples));
    offset += sizeof(numApples);

    for (const auto& apple : apples) {
        memcpy(buffer + offset, &apple, sizeof(apple));
        offset += sizeof(apple);
    }

    auto numPlayers = static_cast<uint16_t>(players.size());
    memcpy(buffer + offset, &numPlayers, sizeof(numPlayers));
    offset += sizeof(numPlayers);

    for (auto& [fd, player] : players) {
        Protocol::PlayerInfo playerInfo{player->id,
            player->score,
            static_cast<uint16_t>(player->body.size())
        };
        memcpy(buffer + offset, &playerInfo, sizeof(playerInfo));
        offset += sizeof(playerInfo);

        for (auto const& segment : player->body) {
            memcpy(buffer + offset, &segment, sizeof(segment));
            offset += sizeof(segment);
        }
    }
}

void Server::broadcastGameStatePacket(uint8_t *buffer, size_t& offset) {
    for (auto& [fd, player] : players) {
        ssize_t bytesSent = send(fd, buffer, offset, MSG_NOSIGNAL); //zapobieganie SIGPIPE

        if (bytesSent == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) { // send() moze wyrzucic blad jesli klient nie bedzie mogl odebrac wiadomosci i zwrosi EAGAIN.
                std::cerr << "Error while sending game state packet to FD: "<< fd << std::endl;
            }
        }
    }
}

void Server::spawnApple() { //zakladamy ze mapa jest 40x30
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distX(1, 38);
    std::uniform_int_distribution<> distY(1, 28);

    Protocol::Apple newApple;
    newApple.x = distX(gen);
    newApple.y = distY(gen);

    apples.push_back(newApple);
}

void Server::respawnPlayer(std::shared_ptr<Player> player) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distX(5, MAP_WIDTH - 5);
    std::uniform_int_distribution<> distY(5, MAP_HEIGHT - 5);

    player->score = 0;
    player->body.clear();

    int16_t startX = distX(gen);
    int16_t startY = distY(gen);


    player->body.push_back(static_cast<Protocol::SnakeSegment>(startX, startY));
}
