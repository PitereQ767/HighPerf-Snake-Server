#pragma once
#include <vector>
#include <sys/epoll.h>

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

        std::pmr::vector<epoll_event> events;

        bool setUpServerSocket();
        bool setNonBlocking();
        void handleNewConnection();
};
