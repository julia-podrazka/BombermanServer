#ifndef SERVER_GAME_H
#define SERVER_GAME_H

#include <set>
#include "server_communication.h"

typedef std::unique_ptr<ServerCommunication> server_communication_ptr;

class ServerGame {

private:

    GameProgramOptions game_options;
    boost::asio::io_context io;
    boost::asio::steady_timer timer;
    Buffer buffer;

    std::map<ClientId, server_communication_ptr> clients;
    std::map<PlayerId, Player> players;
    uint16_t turn;
    std::map<PlayerId, Position> player_positions;
    std::set<std::pair<CoordinateSize, CoordinateSize>> blocks;
    std::map<BombId, Bomb> bombs;
    std::set<std::pair<CoordinateSize, CoordinateSize>> explosions;
    std::map<PlayerId, Score> scores;
    std::vector<ServerMessageToClient::TurnMessage> turn_messages;
    std::vector<ServerMessageToClient::AcceptedPlayerMessage> accepted_players;
    bool is_lobby;

    void clear_turn();

    void clear_game();

public:

    ServerGame(GameProgramOptions &game_options, Buffer &buffer);

    void accept_new_player(boost::asio::ip::tcp::socket socket);

    void process_client_message(ClientMessageToServer &client_message, ClientId client_id);

};

#endif