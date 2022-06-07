#ifndef MESSAGES_H
#define MESSAGES_H

// This file is shared between server and client.

#include <iostream>
#include <variant>
#include <vector>
#include <map>

#ifdef NDEBUG
    const bool debug = false;
#else
    const bool debug = true;
#endif

using PlayerId = uint8_t;
using CoordinateSize = uint16_t;
using Score = uint32_t;
using BombId = uint32_t;
using BombTimer = uint16_t;

// This file contains all the structures that are used in a program including
// structures for massages to/from gui and server.
// If any message have different types of messages, we store the specified type in a
// message_type variable and we store the message of this type/additional
// structures in a variant depending on this type.

enum Direction {
    Up,
    Right,
    Down,
    Left,
};

struct Player {
    std::string name;
    std::string address;
};

struct Position {
    CoordinateSize x{};
    CoordinateSize y{};
};

struct Bomb {
    Position position;
    BombTimer timer{};
};

struct ClientMessageToServer {

    enum MessageType {
        Join,
        PlaceBomb,
        PlaceBlock,
        Move,
    };

    MessageType message_type;

    std::variant<std::string, Direction> message_arguments;

};

struct ServerMessageToClient {

    enum MessageType {
        Hello,
        AcceptedPlayer,
        GameStarted,
        Turn,
        GameEnded,
    };

    struct HelloMessage {
        std::string server_name;
        uint8_t players_count{};
        CoordinateSize size_x{};
        CoordinateSize size_y{};
        uint16_t game_length{};
        uint16_t explosion_radius{};
        BombTimer bomb_timer{};

        HelloMessage() {}
    };

    struct AcceptedPlayerMessage {
        PlayerId id{};
        Player player;
    };

    struct GameStartedMessage {
        std::map<PlayerId, Player> players;
    };

    struct Event {

        enum EventMessage {
            BombPlaced,
            BombExploded,
            PlayerMoved,
            BlockPlaced,
        };

        struct BombPlacedMessage {
            BombId id{};
            Position position;

            BombPlacedMessage() {}
        };

        struct BombExplodedMessage {
            BombId id{};
            std::vector<PlayerId> robots_destroyed;
            std::vector<Position> blocks_destroyed;
        };

        struct PlayerMovedMessage {
            PlayerId id{};
            Position position;
        };

        struct BlockPlacedMessage {
            Position position;
        };

        EventMessage message_type;

        std::variant<BombPlacedMessage, BombExplodedMessage, PlayerMovedMessage, BlockPlacedMessage> message_arguments;

    };

    struct TurnMessage {
        uint16_t turn{};
        std::vector<Event> events;
    };

    struct GameEndedMessage {
        std::map<PlayerId, Score> scores;
    };

    MessageType message_type;

    std::variant<HelloMessage, AcceptedPlayerMessage, GameStartedMessage, TurnMessage, GameEndedMessage> message_arguments;

};

class ClientMessageToGUI {

public:

    enum DrawMessage {
        Lobby,
        Game,
    };

    struct LobbyMessage {
        std::string server_name;
        uint8_t players_count{};
        CoordinateSize size_x{};
        CoordinateSize size_y{};
        uint16_t game_length{};
        uint16_t explosion_radius{};
        BombTimer bomb_timer{};
        std::map<PlayerId, Player> players;

        LobbyMessage() {}
    };

    struct GameMessage {
        std::string server_name;
        CoordinateSize size_x;
        CoordinateSize size_y;
        uint16_t game_length;
        uint16_t turn;
        std::map<PlayerId, Player> players;
        std::map<PlayerId, Position> player_positions;
        std::vector<Position> blocks;
        std::vector<Bomb> bombs;
        std::vector<Position> explosions;
        std::map<PlayerId, Score> scores;
    };

    DrawMessage message_type;

    std::variant<LobbyMessage, GameMessage> message_arguments;

};

struct GUIMessageToClient {

    enum InputMessage {
        PlaceBomb,
        PlaceBlock,
        Move,
    };

    InputMessage message_type;

    Direction message_arguments;

};

#endif
