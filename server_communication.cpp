#include "server_communication.h"
#include "server_game.h"

using namespace std;
using boost::asio::ip::tcp;

ServerCommunication::ServerCommunication(tcp::socket socket, ServerGame *server_game, Buffer &buffer)
    : socket(move(socket)), server_game(server_game), buffer(buffer) {



}
