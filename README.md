# HighPerfSnakeServer

A multiplayer Snake game built on a client-server architecture over TCP. The server is written in C++20 and uses a single-threaded model with epoll and timerfd (Linux), simulating gameplay at 10 TPS. The graphical client is based on SFML 2.6 and ImGui, rendering the game state with motion interpolation at 60 FPS.

## Architecture

```
┌──────────────┐        TCP (port 8080)        ┌──────────────┐
│    Client    │◄──────────────────────────────►│    Server    │
│  SFML/ImGui  │   JoinPacket / MovePacket ──►  │  epoll loop  │
│  60 FPS      │   ◄── GAME_STATE broadcast     │  10 TPS      │
└──────────────┘        PING / PONG             └──────────────┘
```

- **Server** is authoritative — it controls snake movement, collisions, scoring, and respawning. Clients only send join requests and direction changes.
- **Client** receives full game state snapshots and interpolates snake segment positions between server ticks for smooth animation.

## Project Structure

```
tcp_snake_project/
├── CMakeLists.txt              # Root CMake (C++20, optional client)
├── Dockerfile                  # Server image (Ubuntu 22.04, port 8080)
├── shared/
│   ├── CMakeLists.txt
│   └── include/
│       └── Protocol.hpp        # Binary network protocol (packed structs)
├── server/
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── Server.hpp          # Server class, Player struct, map constants
│   └── src/
│       ├── main.cpp            # Server entry point
│       └── Server.cpp          # Game logic, epoll, broadcast
└── client/
    ├── CMakeLists.txt          # SFML + FetchContent (ImGui, ImGui-SFML)
    ├── include/
    │   ├── GameClient.hpp      # Main client class, render loop
    │   ├── NetworkClient.hpp   # TCP communication, state parsing
    │   └── VisualSegment.h     # Segment position interpolation
    ├── src/
    │   ├── main.cpp            # Client entry point
    │   ├── GameClient.cpp      # SFML window, menu, HUD, drawing
    │   └── NetworkClient.cpp   # Connection, recv, sending moves
    └── assets/
        └── Carlito-Bold.ttf    # UI font
```

## Network Protocol

Communication uses raw TCP with Nagle's algorithm disabled (`TCP_NODELAY`). Messages have a fixed binary format defined in `Protocol.hpp` using `#pragma pack(push, 1)`.

| Message Type   | Code | Direction | Description |
|----------------|------|-----------|-------------|
| `JOIN_REQUEST` | 0x01 | C → S     | Nickname (32 bytes) + RGB color |
| `PLAYER_MOVE`  | 0x02 | C → S     | Direction change (dirX, dirY) |
| `GAME_STATE`   | 0x03 | S → C     | Full snapshot: apples + players with segments |
| `PING`         | 0x62 | C → S     | Latency measurement |
| `PONG`         | 0x63 | S → C     | Ping response |

**`GAME_STATE` format:**
1. Message type byte
2. `uint16_t` apple count + array of `Apple` (x, y)
3. `uint16_t` player count
4. Per player: `PlayerInfo` (id, score, length) + segments + nickname (32B) + RGB color (3B)

## Requirements

### Server
- Linux (epoll, timerfd)
- CMake 3.20+
- C++20 compiler (GCC 11+ / Clang 14+)
- pthreads

### Client
- CMake 3.20+
- C++20 compiler
- SFML 2.6 (development packages: `libsfml-dev` or equivalent)
- Network access on first configure (CMake FetchContent downloads ImGui and ImGui-SFML)

## Building

### Full build (server + client)

```bash
mkdir -p build && cd build
cmake ..
cmake --build .
```

### Server only (e.g. headless machine)

```bash
mkdir -p build && cd build
cmake -DBUILD_CLIENT=OFF ..
cmake --build .
```

### Docker (server only)

```bash
docker build -t snake-server .
docker run -p 8080:8080 snake-server
```

## Running

### Server

```bash
./build/server/snake-server
```

The server listens on port **8080**. On startup it logs information about new connections and gameplay events.

### Client

```bash
./build/client/snake-client
```

The client connects to `127.0.0.1:8080` by default. The address and port can be changed in the connection menu within the game window.

## Controls

| Key   | Action     |
|-------|------------|
| ↑ / W | Move up    |
| ↓ / S | Move down  |
| ← / A | Move left  |
| → / D | Move right |

## Gameplay

- The board is **40×30** tiles
- New players spawn at position (10, 10) with a 3-segment snake
- Collecting apples grows the snake and increases score
- Colliding with a wall or your own body triggers a respawn
- No player-vs-player collision

## Technical Details

- **Server:** Single-threaded epoll loop with edge-triggered I/O. A timerfd fires a tick every 100 ms. Client sockets are set to non-blocking mode. A full game state is broadcast to all connected players after every tick.
- **Client:** Renders at 60 FPS with snake segment position interpolation between server ticks (0.1s). Non-blocking recv in the update loop. Ping measurement and network statistics are displayed in the HUD (ImGui).
- **Player identity:** The server assigns a unique ID on connection and maps file descriptors to player structures.
