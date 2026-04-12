#pragma once

#include <cstdint>
#include <string>

class NetworkClient {
public:
   NetworkClient();
   ~NetworkClient();

   bool connectToServer(const std::string& ip, int port);
   void disconnectFromServer();

   void reciveData();

   [[nodiscard]] bool isConnected()const { return connected; }

private:
   bool setNonBlocking(int fd);


   int clientSocketFd{-1};
   bool connected{false};
};