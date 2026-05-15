#include "DatabaseManager.hpp"
#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>

void DatabaseManager::connect() {
    const char* envHost = std::getenv("DB_HOST");
    const char* envName = std::getenv("DB_NAME");
    const char* envUser = std::getenv("DB_USER");
    const char* envPass = std::getenv("DB_PASS");

    std::string dbHost = envHost ? envHost : "db";
    std::string dbName = envName ? envName : "snake_leaderboard";
    std::string dbUser = envUser ? envUser : "admin";
    std::string dbPass = envPass ? envPass : "secret_snake_pass";

    std::string conStr = "host=" + dbHost + " dbname=" + dbName + " user=" + dbUser + " password=" + dbPass;
    conn = std::make_unique<pqxx::connection>(conStr);

    if (conn->is_open()) {
        std::cout << "[DB] Nawizanao polaczenie z baza danych" << std::endl;
        conn->prepare("insert_score", "INSERT INTO scores (nick, score) VALUES ($1, $2)");
    }
}

DatabaseManager::DatabaseManager() {
    int maxRetries = 5;

    while (maxRetries > 0) {
        try {
            connect();
            break;
        }catch (std::exception& e) {
            maxRetries--;
            std :: cerr << "[DB] Baza danych jeszcze nie wstała. Powod: " << e.what() << std::endl;

            if (maxRetries == 0)
            {
                std::cerr << "Nie udalo sie polaczyc z baza danych: "<< e.what() << std::endl;
            }else {
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }
    }
}

void DatabaseManager::saveScore(const std::string &nick, int score) {
    if (!conn || !conn->is_open()) {
        std::cerr << "Connection disconnected! Try to re-connect..." << std::endl;
        try {
            connect();
        }catch (std::exception& e) {
            std::cerr << "Re-connect faild." << std::endl;
        }
    }

    try {
        pqxx::work txn(*conn);
        txn.exec_prepared("insert_score", nick, score);
        txn.commit();
        std::cout << "Result saved: " << nick << " (" << score << ")" << std::endl;
    }catch (std::exception& e) {
        std::cerr << "[DB] Write error: " << e.what() << std::endl;
    }
}
