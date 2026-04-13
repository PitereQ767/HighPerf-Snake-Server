#pragma once

#include <cstdint>
#include <string>
#include "Protocol.hpp"

class NetworkClient {
public:
   NetworkClient();
   ~NetworkClient();

   bool connectToServer(const std::string& ip, int port);
   void disconnectFromServer();

   void reciveData();

   [[nodiscard]] bool isConnected()const { return connected; }

   void sendMoveDirection(Protocol::Direction dirX, Protocol::Direction dirY);

private:
   bool setNonBlocking(int fd);


   int clientSocketFd{-1};
   bool connected{false};
};