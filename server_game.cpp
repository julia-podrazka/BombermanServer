#include "server_game.h"
#include "server_communication.h"

using boost::asio::ip::tcp;

ServerGame::ServerGame(GameProgramOptions &game_options) : game_options(game_options) {



}

void ServerGame::accept_new_player(tcp::socket socket) {

    

}
