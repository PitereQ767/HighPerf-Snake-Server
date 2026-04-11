#include "Server.hpp"

#include <iostream>
#include <ostream>
#include <unistd.h>

#include "Protocol.hpp"

Server::Server(int port) : port(port), events(64) {
    std::cout << "Konstrukyotr serwera (Port: " << port << ")" << std::endl;
}

Server::~Server() {
    std::cout << "Destruktor serwera. Zamykanie gniazd..." << std::endl;

    if (serverFd != -1) {
        close(serverFd);
    }

    if (epoolFd != -1) {
        close(epoolFd);
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
    
}
