#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <sys/epoll.h>

#include "Protocol.hpp"

struct Player {
    uint16_t id;
    int fd;
    int16_t x{0};
    int16_t y{0};
    Protocol::Direction dirX{0};
    Protocol::Direction dirY{0};

    Player(uint16_t id, int fd) : id{id}, fd{fd} {}
};

class Server {
public:
    explicit Server(int port);
    ~Server();

    bool start();
    void run();
private:
    int port;
    int serverFd{-1};
    int epollFd{-1};
    bool isRunning{false};

    std::vector<epoll_event> events;

    uint16_t nextPlayerId{1};
    std::unordered_map<int, std::shared_ptr<Player>> players;

    int timerFd{-1};


    bool setUpServerSocket();
    bool setNonBlocking(int activeFd);
    void handleNewConnection();
    void handleClientData(int clientFd);
    void disconnectClient(int clientFd);

    bool setUpTimer();
    void updateGameState();

    void moveSnakes();
    void processingData(uint8_t* buffer, int clientFd, ssize_t bytesRead);
    void buildGameStatePacket(uint8_t* buffer, size_t& offset);
    void broadcastGameStatePacket(uint8_t* buffer, size_t& offset);
};
