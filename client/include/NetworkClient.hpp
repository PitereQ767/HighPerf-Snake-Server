#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Protocol.hpp"

struct ClientPlayer {
    uint8_t playerId;
    uint16_t score;
    std::vector<Protocol::SnakeSegment> body;
};

class NetworkClient {
public:
   NetworkClient();
   ~NetworkClient();

   bool connectToServer(const std::string& ip, int port);
   void disconnectFromServer();

   void reciveData();

   [[nodiscard]] bool isConnected()const { return connected; }
    const std::vector<ClientPlayer> getPlayers()const { return players; }
    const std::vector<Protocol::Apple> getApples()const { return apples; }

   void sendMoveDirection(Protocol::Direction dirX, Protocol::Direction dirY);

private:
   bool setNonBlocking(int fd);
   void processingClientData(uint8_t* buffer, ssize_t bytesRead);


   int clientSocketFd{-1};
   bool connected{false};

    std::vector<ClientPlayer> players;
    std::vector<Protocol::Apple> apples;
};