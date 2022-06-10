#include "server_game.h"
#include "server_communication.h"

using namespace std;
using boost::asio::ip::tcp;

// In a constructor we initialize the timer that will wait for turn_duration
// time before managing the turn.
ServerGame::ServerGame(GameProgramOptions &game_options, Buffer &buffer)
    : game_options(game_options), buffer(buffer),
      timer(io, boost::asio::chrono::milliseconds(game_options.turn_duration)),
      random(game_options.seed) {

    is_lobby = true;
    turn = 0;

}

void ServerGame::start_game() {

    turn = 0;

    // For every player randomize position and add it to the list of events.
    for (auto& [key, value] : players) {
        Position p;
        p.x = random() % game_options.size_x;
        p.y = random() % game_options.size_y;
        player_positions[key] = p;
        ServerMessageToClient::Event e;
        e.message_type = ServerMessageToClient::Event::PlayerMoved;
        ServerMessageToClient::Event::PlayerMovedMessage e_message;
        e_message.id = key;
        e_message.position = p;
        e.message_arguments = e_message;
        events.push_back(e);
    }

    // Randomize position as many times as there are initial_blocks
    // and add it to the list of events.
    for (uint16_t i = 0; i < game_options.initial_blocks; i++) {
        Position p;
        p.x = random() % game_options.size_x;
        p.y = random() % game_options.size_y;
        blocks.insert(pair<CoordinateSize, CoordinateSize>(p.x, p.y));
        ServerMessageToClient::Event e;
        e.message_type = ServerMessageToClient::Event::BlockPlaced;
        ServerMessageToClient::Event::BlockPlacedMessage e_message;
        e_message.position = p;
        e.message_arguments = e_message;
        events.push_back(e);
    }

    // Prepare TurnMessage.
    ServerMessageToClient server_message;
    server_message.message_type = ServerMessageToClient::Turn;
    ServerMessageToClient::TurnMessage turn_message;
    turn_message.turn = turn;
    turn_message.events = events;
    server_message.message_arguments = turn_message;

    // Send TurnMessage to all clients;
    for (auto& [key, value] : clients) {
        value->send_message(server_message);
    }

    play_game();
    // TODO where to do it? - czy tutaj?
    io.run();

}

void ServerGame::play_game() {

    timer.async_wait([this](const boost::system::error_code &error) {
        if (error) {
            // TODO what to do?
        }
        turn_handler();
    });

}

void ServerGame::turn_handler()  {

    // wybuchamy bomby
    // robimy ruchy graczy
    // wyprodukować TurnMessage
    // zapisać dla potomnych
    // send to all
    clear_turn();
    // TODO check if game ended, if not play_game(), if yes clear_game()
    play_game();

}

void ServerGame::clear_turn() {

    turn++;
    explosions.clear();
    events.clear();
    client_messages.clear();

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
    // TODO send hello + ...

}

void ServerGame::check_game_to_start() {

    if (players.size() == game_options.players_count) {
        is_lobby = false;
        start_game();
    }

}

std::string ServerGame::get_player_address(ClientId client_id) {

    string address = clients.find(client_id)->second->get_client_address();
    uint16_t port = clients.find(client_id)->second->get_client_port();
    char *client_address;
    sprintf(client_address, "%s:%u", address.c_str(), port);
    string string_address(client_address);
    return string_address;

}

void ServerGame::join_player(ClientMessageToServer &client_message, ClientId client_id) {

    // Adding player to players and client_to_player maps.
    Player player;
    player.name = get<string>(client_message.message_arguments);
    player.address = get_player_address(client_id);
    PlayerId player_id = players.size();
    players[player_id] = player;
    client_to_player[client_id] = player_id;

    // Preparing AcceptedPlayerMessage.
    ServerMessageToClient server_message;
    server_message.message_type = ServerMessageToClient::AcceptedPlayer;
    ServerMessageToClient::AcceptedPlayerMessage accepted_player;
    accepted_player.id = player_id;
    accepted_player.player = player;
    server_message.message_arguments = accepted_player;

    // Sending AcceptedPlayerMessage to all clients.
    for (auto& [key, value] : clients) {
        value->send_message(server_message);
    }

    // Save AcceptedPlayerMessage.
    accepted_players.push_back(server_message);

    // Set score of a player to 0.
    scores[player_id] = 0;

}

void ServerGame::process_client_message(ClientMessageToServer &client_message, ClientId client_id) {

    // If the message is not a Join message and it is send during game, it is saved
    // in client_messages, because only the latest message in a turn counts.
    switch (client_message.message_type) {
        case ClientMessageToServer::Join: {
            // Join messages during game and from already joined players are ignored.
            if (is_lobby && (client_to_player.find(client_id) == client_to_player.end())) {
                join_player(client_message, client_id);
                check_game_to_start();
            }
            break;
        } default: {
            // If there is no game and client is not a player the move is ignored.
            if (!is_lobby && (client_to_player.find(client_id) != client_to_player.end())) {
                client_messages[client_to_player.find(client_id)->second] = client_message;
            }
            break;
        }
    }

}
