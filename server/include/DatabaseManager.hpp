#pragma once
#include <memory>
#include <pqxx/pqxx>
#include <string>

class DatabaseManager {
private:
    std::unique_ptr<pqxx::connection> conn;
    void connect();

public:
    DatabaseManager();
    void saveScore(const std::string& nick, int score);

};
