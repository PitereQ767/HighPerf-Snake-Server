#include "Protocol.hpp"
#include "NetworkClient.hpp"

#include <fcntl.h>
#include <iostream>
#include <ostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

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

bool NetworkClient::connectToServer(const std::string& ip, int port) {
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

    std::cout << "Connected to server IP: " << ip << " and port: " << port << std::endl;
    connected = true;

    if (!setNonBlocking(clientSocketFd)) {
        std::cerr << "Failed to set non-blocking socket: "<< clientSocketFd << std::endl;
    }

    uint8_t helloPacket = static_cast<uint8_t>(Protocol::MessageType::JOIN_REQUEST);

    if (send(clientSocketFd, &helloPacket, sizeof(helloPacket), MSG_NOSIGNAL) < 0) {
        std::cerr << "Failed to send JOIN REQUEST." << std::endl;
        disconnectFromServer();
        return false;
    }

    return true;
}

bool NetworkClient::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) return false;
    return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1);
}

void NetworkClient::reciveData() {
    if (!connected) return;

    uint8_t buffer[1024];

    ssize_t numBytes = recv(clientSocketFd, buffer, sizeof(buffer), 0);

    if (numBytes > 0) {
        std::cout << "Recived " << numBytes << " bytes from client" << std::endl;
    }else if (numBytes == 0) {
        std::cout << "Server closed connection" << std::endl;
        disconnectFromServer();
    }else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            disconnectFromServer();
        }
    }
}

void NetworkClient::sendMoveDirection(int8_t dirX, int8_t dirY) {
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

