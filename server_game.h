#ifndef SERVER_GAME_H
#define SERVER_GAME_H

#include <random>
#include <cstdint>
#include <set>
#include "server_communication.h"

typedef std::unique_ptr<ServerCommunication> server_communication_ptr;

class ServerGame {

private:

    GameProgramOptions game_options;
    boost::asio::io_context io;
    boost::asio::steady_timer timer;
    Buffer buffer;
    std::minstd_rand random;

    std::map<ClientId, server_communication_ptr> clients;
    std::map<PlayerId, Player> players;
    uint16_t turn;
    std::map<PlayerId, Position> player_positions;
    std::set<std::pair<CoordinateSize, CoordinateSize>> blocks;
    std::map<BombId, Bomb> bombs;
    std::vector<BombId> exploded_bombs;
    std::set<std::pair<CoordinateSize, CoordinateSize>> explosions;
    std::set<PlayerId> killed_robots;
    std::map<PlayerId, Score> scores;
    std::vector<ServerMessageToClient> turn_messages;
    std::vector<ServerMessageToClient> accepted_players;
    ServerMessageToClient game_started;
    std::map<ClientId, PlayerId> client_to_player;
    std::map<PlayerId, ClientMessageToServer> client_messages;
    std::vector<ServerMessageToClient::Event> events;
    bool is_lobby;

    void start_game();

    void play_game();

    void explode_bomb(const BombId &key, Bomb &value);

    void check_bombs();

    void turn_handler();

    void clear_turn();

    void clear_game();

    void check_game_to_start();

    std::string get_player_address(ClientId client_id);

    void join_player(ClientMessageToServer &client_message, ClientId client_id);

public:

    ServerGame(GameProgramOptions &game_options, Buffer &buffer);

    void accept_new_player(boost::asio::ip::tcp::socket socket);

    void process_client_message(ClientMessageToServer &client_message, ClientId client_id);

};

#endif