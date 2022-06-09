#include "server_game.h"
#include "server_communication.h"

using namespace std;
using boost::asio::ip::tcp;

// In a constructor we initialize the timer that will wait for turn_duration
// time before managing the turn.
ServerGame::ServerGame(GameProgramOptions &game_options, Buffer &buffer)
    : game_options(game_options), buffer(buffer),
      timer(io, boost::asio::chrono::milliseconds(game_options.turn_duration)) {

    is_lobby = true;
    turn = 0;

}

void ServerGame::clear_turn() {

    turn++;
    explosions.clear();

}

void ServerGame::clear_game() {

    is_lobby = true;
    turn = 0;
    players.clear();
    player_positions.clear();
    blocks.clear();
    bombs.clear();
    explosions.clear();
    scores.clear();
    turn_messages.clear();
    accepted_players.clear();

}

void ServerGame::accept_new_player(tcp::socket socket) {

    // Inserts a unique pointer to new client in a map of clients. ClientId is
    // the next available number.
    size_t number_of_clients = clients.size();
    clients[number_of_clients] = make_unique<ServerCommunication>(std::move(socket), this, buffer, number_of_clients);

}

void ServerGame::process_client_message(ClientMessageToServer &client_message, ClientId client_id) {

    switch (client_message.message_type) {
        case ClientMessageToServer::Join: {
//            auto
//            string address =
            break;
        } case ClientMessageToServer::PlaceBomb: {

            break;
        } case ClientMessageToServer::PlaceBlock: {

            break;
        } case ClientMessageToServer::Move: {

            break;
        } default: {
            break;
        }
    }

}
