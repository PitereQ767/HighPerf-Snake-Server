#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Protocol.hpp"

class NetworkClient {
public:
   NetworkClient();
   ~NetworkClient();

   bool connectToServer(const std::string& ip, int port);
   void disconnectFromServer();

   void reciveData();

   [[nodiscard]] bool isConnected()const { return connected; }
    const std::vector<Protocol::PlayerState> getPlayers()const { return players; }

   void sendMoveDirection(Protocol::Direction dirX, Protocol::Direction dirY);

private:
   bool setNonBlocking(int fd);
   void processingClientData(uint8_t* buffer, ssize_t bytesRead);


   int clientSocketFd{-1};
   bool connected{false};

   std::vector<Protocol::PlayerState> players;
};