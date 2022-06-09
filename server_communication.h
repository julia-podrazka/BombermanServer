#ifndef SERVER_COMMUNICATION_H
#define SERVER_COMMUNICATION_H

#include <boost/asio.hpp>
#include "buffer.h"
#include "messages.h"

class ServerGame;

class ServerCommunication {

private:

    ServerGame *server_game;
    Buffer buffer;
    boost::asio::ip::tcp::socket socket;

public:

    ServerCommunication(boost::asio::ip::tcp::socket socket, ServerGame *server_game, Buffer &buffer);

};

#endif
