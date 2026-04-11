#pragma once
#include <cstdint>

namespace Protocol {
    enum class MessageType : uint8_t {
        JOIN_REQUEST = 0x01,
        JOIN_ACCEPT = 0x02,
        PLAYER_MOVE = 0x03,
        SPAWN_FRUIT = 0x04,
        PLAYER_DIE = 0x05,
        GAME_STATE = 0x06
    };

    #pragma pack(push, 1)

    struct MovePacket {
        MessageType type;
        uint16_t playerId;
        int8_t dirX;
        int8_t dirY;
    };

    struct FruitSpawnPacket {
        MessageType type;
        uint16_t x;
        uint16_t y;
    };

    struct PlayerState {
        uint16_t playerId;
        int16_t x;
        int16_t y;
    };

    #pragma pack(pop)
}
