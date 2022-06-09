#ifndef CLIENT_GAME_H
#define CLIENT_GAME_H

#include <iostream>
#include <set>
#include "messages.h"

class ClientGame {

private:

    // This class contains all the information about the game that are needed
    // to be sent to gui.
    std::string server_name;
    uint8_t players_count;
    CoordinateSize size_x;
    CoordinateSize size_y;
    uint16_t game_length;
    uint16_t explosion_radius;
    BombTimer bomb_timer;
    std::map<PlayerId, Player> players;
    uint16_t turn{};
    std::map<PlayerId, Position> player_positions;
    std::set<std::pair<CoordinateSize, CoordinateSize>> blocks;
    std::map<BombId, Bomb> bombs;
    std::set<std::pair<CoordinateSize, CoordinateSize>> explosions;
    std::map<PlayerId, Score> scores;

    bool is_lobby; // determines whether we are in a lobby or in a game
    std::set<PlayerId> killed_robots; // stores playerIds of robots that were killed in a turn
    // stores blocks to be destroyed after reading all events
    std::set<std::pair<CoordinateSize, CoordinateSize>> blocks_to_destroy;

    // Writes LobbyMessage in client_message argument.
    void prepare_lobby_message(ClientMessageToGUI &client_message);

    // Copies a set of pairs with coordinates into a vector of Position.
    static std::vector<Position> copy_set_to_vector(const std::set<std::pair<CoordinateSize, CoordinateSize>> &set_to_copy);

    // Copies a map with BombId and Bomb to a vector of Bombs.
    std::vector<Bomb> copy_map_to_vector(const std::map<BombId, Bomb> &bombs);

    // Writes GameMessage in a client_message argument.
    void prepare_game_message(ClientMessageToGUI &client_message);

    // Places bombs that are written in bomb_placed structure.
    void place_bomb(ServerMessageToClient::Event::BombPlacedMessage &bomb_placed);

    // Explodes bombs that are written in bomb_exploded structure.
    void explode_bomb(ServerMessageToClient::Event::BombExplodedMessage &bomb_exploded);

    // Moves players that are written in player_moved structure.
    void move_player(ServerMessageToClient::Event::PlayerMovedMessage &player_moved);

    // Places blocks that are written in block_placed structure.
    void place_block(ServerMessageToClient::Event::BlockPlacedMessage &block_placed);

    // Adds positions to explosions set after an explosion of a bomb.
    void explosion_from_bomb(ServerMessageToClient::Event::BombExplodedMessage &bomb_exploded);

public:

    ClientGame();

    [[nodiscard]] bool get_is_lobby() const;

    // Gets information and writes it into its attributes from hello_message from server.
    void get_hello_message(const ServerMessageToClient::HelloMessage &hello_message);

    // Changes the game status from lobby to game with players that are in all_players map.
    void change_game_status_to_started(std::map<PlayerId, Player> &all_players);

    // Changes the game status from game to lobby, prepares a LobbyMessage and clears necessary structures.
    void change_game_status_to_ended(ClientMessageToGUI &client_message);

    // Writes message to GUI in a client_message structure.
    void prepare_gui_message(ClientMessageToGUI &client_message);

    // Gets information and writes it into its attributes from turn_message from server.
    void get_turn_message(ServerMessageToClient::TurnMessage &turn_message);

    // Accepts a new player from accepted_player structure.
    void accept_player(const ServerMessageToClient::AcceptedPlayerMessage &accepted_player);

};

#endif
