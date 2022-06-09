#ifndef SERVER_GAME_H
#define SERVER_GAME_H

#include "server_communication.h"

class ServerGame {

private:

    GameProgramOptions game_options;

public:

    ServerGame(GameProgramOptions &game_options);

};

#endif