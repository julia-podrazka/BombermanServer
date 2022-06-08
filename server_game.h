#ifndef GAME_H
#define GAME_H

#include "messages.h"

class ServerGame {

private:

    GameProgramOptions game_options;

public:

    ServerGame(GameProgramOptions &game_options);

};

#endif