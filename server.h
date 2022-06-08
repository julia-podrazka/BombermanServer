#ifndef SERVER_H
#define SERVER_H

#include "server_game.h"

class Server {

private:

    GameProgramOptions game_options;
    uint16_t port{};

public:

    Server();

    uint16_t get_port() const {
        return port;
    }

    GameProgramOptions &get_game_options() {
        return game_options;
    }

    void parse_program_options(int argc, char *argv[]);

};

#endif
