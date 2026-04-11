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

        uint16_t newId = nextPlayerId++;
        auto newPlayer = std::make_shared<Player>(newId, clientFd);

        newPlayer->x = 100; // tymczasowa pozycja startowa
        newPlayer->y = 100;

        players[clientFd] = newPlayer;

        std::cout << "New player: " << clientFd <<  " IP: " << inet_ntoa(clientAddr.sin_addr) << std::endl;
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

        if (bytesRead == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }else {
                disconnectClient(clientFd);
            }
        }else if (bytesRead == 0) {
            disconnectClient(clientFd);
        }else {
            std::cout << "Odebrano: " << bytesRead << " bajtow." << std::endl;
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
        if (player->dirX != 0 || player->dirY != 0) {
            player->x += player->dirX;
            player->y += player->dirY;

            //do zaimplemontowania kolizje i jablka
        }
    }
}

void Server::buildGameStatePacket(uint8_t* buffer, size_t& offset) {

    buffer[offset] = static_cast<uint8_t>(Protocol::MessageType::GAME_STATE);
    offset++;

    auto numPlayers = static_cast<uint16_t>(players.size());
    memcpy(buffer + offset, &numPlayers, sizeof(numPlayers));
    offset += sizeof(numPlayers);

    for (auto& [fd, player] : players) {
        Protocol::PlayerState playerState{player->id, player->x, player->y};
        memcpy(buffer + offset, &playerState, sizeof(playerState));
        offset += sizeof(playerState);
    }
}

void Server::broadcastGameStatePacket(uint8_t *buffer, size_t& offset) {
    for (auto& [fd, player] : players) {
        ssize_t bytesSent = send(fd, buffer, offset, MSG_NOSIGNAL);

        if (bytesSent == -1) {
            if (errno != EAGAIN || errno != EWOULDBLOCK) { // send() moze wyrzucic blad jesli klient nie bedzie mogl odebrac wiadomosci i zwrosi EAGAIN.
                std::cerr << "Error while sending game state packet to FD: "<< fd << std::endl;
            }
        }
    }
}
