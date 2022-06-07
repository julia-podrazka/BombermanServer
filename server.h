#ifndef SERVER_H
#define SERVER_H

struct GameProgramOptions {

    uint16_t bomb_timer;
    uint8_t players_count;
    uint64_t turn_duration;
    uint16_t explosion_radius;
    uint16_t initial_blocks;
    uint16_t game_length;
    std::string server_name;
    uint32_t seed;
    uint16_t size_x;
    uint16_t size_y;

};

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
