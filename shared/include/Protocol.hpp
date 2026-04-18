#pragma once
#include <cstdint>


namespace Protocol {
    #pragma pack(push, 1)

    enum class MessageType : uint8_t {
        JOIN_REQUEST = 0x01,
        PLAYER_MOVE = 0x02,
        GAME_STATE = 0x03,
        PING = 0x62,
        PONG = 0x63
    };

    enum class Direction : int8_t {
        UP = -1,
        DOWN = 1,
        LEFT = -1,
        RIGHT = 1,
        NEUTRAL = 0
    };

    struct MovePacket {
        MessageType type;
        uint16_t playerId;
        Direction dirX;
        Direction dirY;
    };

    struct Apple {
        uint16_t x;
        uint16_t y;
    };

    struct SnakeSegment {
        int16_t x;
        int16_t y;
    };

    struct Color {
        uint8_t r, g, b;
    };

    struct PlayerInfo {
        uint16_t id;
        uint16_t score;
        uint16_t length;
        char NickName[32];
        Color color;
    };

    struct JoinPacket {
        MessageType type = MessageType::JOIN_REQUEST;
        char NickName[32];
        uint8_t colorR;
        uint8_t colorG;
        uint8_t colorB;
    };

    #pragma pack(pop)
}
