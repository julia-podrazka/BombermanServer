#ifndef SERVER_H
#define SERVER_H

#include "server_game.h"

class Server {

private:

    GameProgramOptions game_options;
    uint16_t port{};

public:

    Server();

    [[nodiscard]] uint16_t get_port() const {
        return port;
    }

    GameProgramOptions &get_game_options() {
        return game_options;
    }

    void parse_program_options(int argc, char *argv[]);

};

class Acceptor {

private:

    boost::asio::ip::tcp::acceptor acceptor;
    ServerGame server_game;

public:

    Acceptor(boost::asio::io_context &io_context, const boost::asio::ip::tcp::endpoint &endpoint,
             GameProgramOptions &options, Buffer &buffer);

    void accept_players();

};

#endif
