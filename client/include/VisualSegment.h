#pragma once
#include <cstdint>
#include <queue>
#include <vector>
#include <SFML/System/Vector2.hpp>

struct VisualSegment {
    sf::Vector2f currentPos;
    sf::Vector2f lastTargetPos;
    std::queue<sf::Vector2f> pathQueue;
};

class VisualPlayer {
public:
    uint16_t playerId;

    std::vector<VisualSegment> visualBody;

};
