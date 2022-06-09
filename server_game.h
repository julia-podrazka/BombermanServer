#ifndef SERVER_GAME_H
#define SERVER_GAME_H

#include "server_communication.h"

typedef std::unique_ptr<ServerCommunication> server_communication_ptr;
// Because PlayerId is only assigned to the clients that sent Join,
// all clients will have their own ClientId, unrelated to PlayerId.
using ClientId = uint8_t;

class ServerGame {

private:

    GameProgramOptions game_options;
    std::map<ClientId, server_communication_ptr> clients;

public:

    ServerGame(GameProgramOptions &game_options);

    void accept_new_player(boost::asio::ip::tcp::socket socket);

};

#endif