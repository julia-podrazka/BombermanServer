#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>

class Client {

private:

    std::string player_name;
    uint16_t port{};
    std::string gui_address;
    std::string server_address;

public:

    Client();

    std::string &get_player_name() {

        return player_name;

    }

    uint16_t get_port() const {

        return port;

    }

    std::string &get_gui_address() {

        return gui_address;

    }

    std::string &get_server_address() {

        return server_address;

    }

    void parse_program_options(int argc, char *argv[]);

};

#endif
