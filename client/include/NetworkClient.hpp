#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

#include "Protocol.hpp"

struct ClientPlayer {
    uint16_t playerId;
    uint16_t score;
    std::vector<Protocol::SnakeSegment> body;
    std::string nick;
    Protocol::Color color;
};

class NetworkClient {
public:
   NetworkClient();
   ~NetworkClient();

   bool connectToServer(const std::string& ip, int port, const char* nick, const float *color);
   void disconnectFromServer();

   void reciveData();

   [[nodiscard]] bool isConnected()const { return connected; }
    const std::vector<ClientPlayer> getPlayers()const { return players; }
    const std::vector<Protocol::Apple> getApples()const { return apples; }

   void sendMoveDirection(Protocol::Direction dirX, Protocol::Direction dirY);
    int getPing(){return currentPing;}
    int getPacketsPerSecond(){return lastPacketsPerSecond;}
    float getLastBytePerSecond(){return lastBytesPerSecond;}

    void updateNetworkState();

private:
   bool setNonBlocking(int fd);
   size_t processingClientData(uint8_t* buffer, ssize_t bytesRead);
    void buildJoinPacket(Protocol::JoinPacket& joinPacket, const char* nick, const float* color);


   int clientSocketFd{-1};
   bool connected{false};

    std::vector<ClientPlayer> players;
    std::vector<Protocol::Apple> apples;

    int currentBytes = 0;
    int currentPackets = 0;
    int lastBytesPerSecond = 0;
    int lastPacketsPerSecond = 0;

    int currentPing = 0;
    using TimePoint = std::chrono::steady_clock::time_point;
    TimePoint lastStatTime;
    TimePoint lastPingSentTime;
    TimePoint pingRequestTime;
};