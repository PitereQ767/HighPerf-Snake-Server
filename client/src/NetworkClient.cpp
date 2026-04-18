#include "Protocol.hpp"
#include "NetworkClient.hpp"

#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <ostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

NetworkClient::NetworkClient() {

}

NetworkClient::~NetworkClient() {
    disconnectFromServer();
}

void NetworkClient::disconnectFromServer() {
    if (connected && clientSocketFd != -1) {
        close(clientSocketFd);
        clientSocketFd = -1;
        connected = false;
        std::cout << "Disconnected from the server" << std::endl;
    }
}

bool NetworkClient::connectToServer(const std::string& ip, int port, const char* nick, const float* color) {
    clientSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocketFd == -1) {
        std::cout << "Failed to create a socket (clientSocket)" << std::endl;
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cout << ip << std::endl;
        std::cout << "Invalid IP address to server" << std::endl;
        return false;
    }

    std::cout << "Try to connect to server IP: " << ip << " and port: " << port << std::endl;

    if (connect(clientSocketFd, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cout << "Failed to connect to server" << std::endl;
        close(clientSocketFd);
        return false;
    }

    int flag = 1;
    setsockopt(clientSocketFd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

    std::cout << "Connected to server IP: " << ip << " and port: " << port << std::endl;
    connected = true;

    if (!setNonBlocking(clientSocketFd)) {
        std::cerr << "Failed to set non-blocking socket: "<< clientSocketFd << std::endl;
    }

    Protocol::JoinPacket join_packet;
    buildJoinPacket(join_packet, nick, color);

    if (send(clientSocketFd, &join_packet, sizeof(Protocol::JoinPacket), MSG_NOSIGNAL) < 0) {
        std::cerr << "Failed to send JOIN REQUEST." << std::endl;
        disconnectFromServer();
        return false;
    }

    lastStatTime = std::chrono::steady_clock::now();
    lastPingSentTime = std::chrono::steady_clock::now();

    return true;
}

void NetworkClient::buildJoinPacket(Protocol::JoinPacket &joinPacket, const char *nick, const float *color) {
    std::strncpy(joinPacket.NickName, nick, sizeof(joinPacket.NickName) - 1);
    joinPacket.NickName[sizeof(joinPacket.NickName) - 1] = '\0';

    joinPacket.colorR = static_cast<uint8_t>(color[0] * 255.0f);
    joinPacket.colorG = static_cast<uint8_t>(color[1] * 255.0f);
    joinPacket.colorB = static_cast<uint8_t>(color[2] * 255.0f);
}

bool NetworkClient::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) return false;
    return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1);
}

size_t NetworkClient::processingClientData(uint8_t *buffer, ssize_t bytesRead) {
    auto messageType = static_cast<Protocol::MessageType>(buffer[0]);

    if (messageType == Protocol::MessageType::GAME_STATE) {
        size_t offset{1};

        if (bytesRead < 5) return 0;

        uint16_t numApples;
        memcpy(&numApples, buffer + offset, sizeof(numApples));
        offset += sizeof(numApples);

        apples.clear();

        for (int i = 0; i < numApples; i++) {
            Protocol::Apple apple;

            if (offset + sizeof(apple) > bytesRead) break;

            memcpy(&apple, buffer + offset, sizeof(apple));
            offset += sizeof(apple);
            apples.push_back(apple);
        }

        uint16_t numPlayers;
        memcpy(&numPlayers, buffer + offset, sizeof(numPlayers));
        offset += sizeof(numPlayers);

        players.clear();

        for (int i = 0; i < numPlayers; i++) {
            Protocol::PlayerInfo pInfo;

            if (offset + sizeof(pInfo) > bytesRead) break;

            memcpy(&pInfo, buffer + offset, sizeof(pInfo));
            offset += sizeof(pInfo);

            ClientPlayer cp;
            cp.playerId = pInfo.id;
            cp.score = pInfo.score;

            for (int j = 0; j < pInfo.length; j++) {
                Protocol::SnakeSegment segment;

                if (offset + sizeof(segment) > bytesRead) break;

                memcpy(&segment, buffer + offset, sizeof(segment));
                offset += sizeof(segment);
                cp.body.push_back(segment);
            }

            char nickBuffer[32];
            memcpy(nickBuffer, buffer + offset, sizeof(pInfo.NickName));
            nickBuffer[31] = '\0';
            cp.nick = std::string(nickBuffer);
            offset += sizeof(pInfo.NickName);

            memcpy(&cp.color.r, buffer + offset, sizeof(cp.color.r));
            offset += sizeof(cp.color.r);
            memcpy(&cp.color.g, buffer + offset, sizeof(cp.color.g));
            offset += sizeof(cp.color.g);
            memcpy(&cp.color.b, buffer + offset, sizeof(cp.color.b));
            offset += sizeof(cp.color.b);

            players.push_back(cp);
        }
        return offset;
    }
}

void NetworkClient::reciveData() {
    if (!connected) return;

    uint8_t buffer[8192];

    while (true) {
        ssize_t numBytes = recv(clientSocketFd, buffer, sizeof(buffer), 0);

        if (numBytes > 0) {
            size_t offset = 0;
            currentBytes += numBytes;
            currentPackets++;

            while (offset < numBytes) {
                auto type = static_cast<Protocol::MessageType>(buffer[offset]);
                if (type == Protocol::MessageType::GAME_STATE) {
                    size_t consumed = processingClientData(buffer + offset, numBytes - offset);
                    if (consumed == 0) break;
                    offset += consumed;
                }else if (type == Protocol::MessageType::PONG) {
                    auto now = std::chrono::steady_clock::now();
                    currentPing = std::chrono::duration_cast<std::chrono::microseconds>(now - pingRequestTime).count();
                    return;
                }else {
                    break;
                }
            }
            std::cout << "Recived " << numBytes << " bytes from server" << std::endl;
        }else if (numBytes == 0) {
            std::cout << "Server closed connection" << std::endl;
            disconnectFromServer();
            return;
        }else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                disconnectFromServer();
            }
            return;
        }
    }
}

void NetworkClient::sendMoveDirection(Protocol::Direction dirX, Protocol::Direction dirY) {
    if (!connected || clientSocketFd == -1) return;

    Protocol::MovePacket packet{};
    packet.type = Protocol::MessageType::PLAYER_MOVE;
    packet.playerId = 0;
    packet.dirX = dirX;
    packet.dirY = dirY;

    ssize_t numBytes = send(clientSocketFd, &packet, sizeof(packet), 0);

    if (numBytes < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Failed to send player move." << std::endl;
            disconnectFromServer();
        }
    }
}

void NetworkClient::updateNetworkState() {
    if (!connected) return;

    auto now = std::chrono::steady_clock::now();

    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastStatTime).count() >= 1) {
        lastBytesPerSecond = currentBytes;
        lastPacketsPerSecond = currentPackets;

        currentBytes = 0;
        currentPackets = 0;
        lastStatTime = now;
    }

    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastPingSentTime).count() >= 1) {
        uint8_t pingPacket = static_cast<uint8_t>(Protocol::MessageType::PING);
        send(clientSocketFd, &pingPacket, sizeof(pingPacket), MSG_NOSIGNAL);

        pingRequestTime = std::chrono::steady_clock::now();
        lastPingSentTime = now;
    }
}

