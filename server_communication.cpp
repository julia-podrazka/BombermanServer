#include "server_communication.h"
#include "server_game.h"

using namespace std;
using boost::asio::ip::tcp;

ServerCommunication::ServerCommunication(tcp::socket socket, ServerGame *server_game, Buffer &buffer, size_t client_id)
    : socket(move(socket)), server_game(server_game), buffer(buffer), client_id(static_cast<ClientId>(client_id)) {

    socket.set_option(tcp::no_delay(true));

}
