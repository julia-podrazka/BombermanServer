#include "server_game.h"
#include "server_communication.h"

using namespace std;
using boost::asio::ip::tcp;

// In a constructor we initialize the timer that will wait for turn_duration
// time before managing the turn.
ServerGame::ServerGame(GameProgramOptions &game_options, Buffer &buffer, boost::asio::io_context &io_context)
    : game_options(game_options),
      timer(io_context, boost::asio::chrono::milliseconds(game_options.turn_duration)),
      buffer(buffer), random(game_options.seed) {

    is_lobby = true;
    turn = 0;

}

Position ServerGame::randomize_position() {

    Position p;
    p.x = static_cast<CoordinateSize>(random() % game_options.size_x);
    p.y = static_cast<CoordinateSize>(random() % game_options.size_y);
    return p;

}

void ServerGame::start_game() {

    turn = 0;

    // For every player randomize position and add it to the list of events.
    for (auto& [key, value] : players) {
        Position p = randomize_position();
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
        Position p = randomize_position();
        p.y = static_cast<CoordinateSize>(random() % game_options.size_y);
        blocks.insert(pair<CoordinateSize, CoordinateSize>(p.x, p.y));
        ServerMessageToClient::Event e;
        e.message_type = ServerMessageToClient::Event::BlockPlaced;
        ServerMessageToClient::Event::BlockPlacedMessage e_message;
        e_message.position = p;
        e.message_arguments = e_message;
        events.push_back(e);
    }

    // Prepare GameStartedMessage.
    ServerMessageToClient server_message_started;
    server_message_started.message_type = ServerMessageToClient::GameStarted;
    ServerMessageToClient::GameStartedMessage game_message;
    game_message.players = players;
    server_message_started.message_arguments = game_message;

    // Prepare TurnMessage.
    ServerMessageToClient server_message;
    server_message.message_type = ServerMessageToClient::Turn;
    ServerMessageToClient::TurnMessage turn_message;
    turn_message.turn = turn;
    turn_message.events = events;
    server_message.message_arguments = turn_message;

    // Send GameStartedMessage and TurnMessage to all clients.
    for (auto& [key, value] : clients) {
        value->send_message(server_message_started);
        value->send_message(server_message);
    }

    // Save GameStartedMessage for spectators.
    game_started = server_message_started;

    // Clearing attributes for first turn.
    clear_turn();

    // Starting turns.
    play_game();

}

void ServerGame::play_game() {

    timer.expires_from_now(boost::asio::chrono::milliseconds(game_options.turn_duration));
    timer.async_wait([this](const boost::system::error_code &error) {
        if (error) {
            // TODO what to do?
        }
        turn_handler();
    });

}

void ServerGame::explode_bomb(const BombId &key, Bomb &value) {

    explosions.insert(pair<CoordinateSize, CoordinateSize>(value.position.x, value.position.y));
    // Adds explosions from above, below, on the right/left to the exploding bomb depending
    // on explosion_radius and whether there is a block blocking the way.
    if (blocks.count(pair<CoordinateSize, CoordinateSize>(value.position.x, value.position.y)) == 0) {
        if (value.position.y != game_options.size_y - 1) {
            for (CoordinateSize i = value.position.y + 1, k = 0;
                 (k < game_options.explosion_radius && i < game_options.size_y); i++, k++) {
                explosions.insert(pair<CoordinateSize, CoordinateSize>(value.position.x, i));
                if (blocks.count(pair<CoordinateSize, CoordinateSize>(value.position.x, i)) != 0)
                    break;
            }
        }
        if (value.position.x != game_options.size_x - 1) {
            for (CoordinateSize i = value.position.x + 1, k = 0;
                 (k < game_options.explosion_radius && i < game_options.size_x); i++, k++) {
                explosions.insert(pair<CoordinateSize, CoordinateSize>(i, value.position.y));
                if (blocks.count(pair<CoordinateSize, CoordinateSize>(i, value.position.y)) != 0)
                    break;
            }
        }
        if (value.position.y != 0) {
            for (CoordinateSize i = value.position.y - 1, k = 0; k < game_options.explosion_radius; i--, k++) {
                explosions.insert(pair<CoordinateSize, CoordinateSize>(value.position.x, i));
                if (blocks.count(pair<CoordinateSize, CoordinateSize>(value.position.x, i)) != 0)
                    break;
                if (i == 0)
                    break;
            }
        }
        if (value.position.x != 0) {
            for (CoordinateSize i = value.position.x - 1, k = 0; k < game_options.explosion_radius; i--, k++) {
                explosions.insert(pair<CoordinateSize, CoordinateSize>(i, value.position.y));
                if (blocks.count(pair<CoordinateSize, CoordinateSize>(i, value.position.y)) != 0)
                    break;
                if (i == 0)
                    break;
            }
        }
    }
    exploded_bombs.push_back(key);

}

void ServerGame::check_bombs() {

    for (auto&[key, value] : bombs) {
        value.timer -= 1;
        if (value.timer == 0) {
            explosions.clear();
            // explode_bomb will mark in explosions which positions where
            // destroyed by bomb.
            explode_bomb(key, value);
            ServerMessageToClient::Event event;
            event.message_type = ServerMessageToClient::Event::BombExploded;
            ServerMessageToClient::Event::BombExplodedMessage bomb;
            bomb.id = key;
            for (auto &explosion : explosions) {
                // Block should be destroyed.
                if (blocks.contains(pair<CoordinateSize, CoordinateSize>(explosion.first,
                        explosion.second))) {
                    Position p;
                    p.x = explosion.first;
                    p.y = explosion.second;
                    bomb.blocks_destroyed.push_back(p);
                    blocks.erase(pair<CoordinateSize, CoordinateSize>(explosion.first,
                                                                      explosion.second));
                }
                // We check if each player was killed by a bomb.
                for (auto &[p_key, p_value] : player_positions) {
                    if (p_value.x == explosion.first && p_value.y == explosion.second) {
                        killed_robots.insert(p_key);
                        bomb.robots_destroyed.push_back(p_key);
                    }
                }
            }
            event.message_arguments = bomb;
            events.push_back(event);
        }
    }

    // Erase every bomb that exploded.
    for (auto &exploded : exploded_bombs)
        bombs.erase(exploded);

}

void ServerGame::make_players_move(ClientMessageToServer &client_message, PlayerId player_id) {

    Position p = player_positions.find(player_id)->second;
    switch (client_message.message_type) {
        case ClientMessageToServer::Move: {
            Direction d = get<Direction>(client_message.message_arguments);
            Position new_p;
            bool can_move = false;
            // Checking if a player can move, saving his new position and
            // adding new event PlayerMoved.
            if (d == Direction::Up && p.y != (game_options.size_y - 1)) {
                new_p.x = p.x;
                new_p.y = p.y + 1;
                can_move = true;
            } else if (d == Direction::Down && p.y != 0) {
                new_p.x = p.x;
                new_p.y = p.y - 1;
                can_move = true;
            } else if (d == Direction::Left && p.x != 0) {
                new_p.x = p.x - 1;
                new_p.y = p.y;
                can_move = true;
            } else if (d == Direction::Right && p.x != (game_options.size_x - 1)) {
                new_p.x = p.x + 1;
                new_p.y = p.y;
                can_move = true;
            }
            // Player can't move outside of game board and on a block.
            if (can_move && !blocks.contains(pair<CoordinateSize, CoordinateSize>(new_p.x, new_p.y))) {
                player_positions[player_id] = new_p;
                ServerMessageToClient::Event event;
                event.message_type = ServerMessageToClient::Event::PlayerMoved;
                ServerMessageToClient::Event::PlayerMovedMessage player;
                player.id = player_id;
                player.position = new_p;
                event.message_arguments = player;
                events.push_back(event);
            }
            break;
        } case ClientMessageToServer::PlaceBlock: {
            if (!blocks.contains(pair<CoordinateSize, CoordinateSize>(p.x, p.y))) {
                blocks.insert(pair<CoordinateSize, CoordinateSize>(p.x, p.y));
                ServerMessageToClient::Event event;
                event.message_type = ServerMessageToClient::Event::BlockPlaced;
                ServerMessageToClient::Event::BlockPlacedMessage block;
                block.position = p;
                event.message_arguments = block;
                events.push_back(event);
            }
            break;
        } case ClientMessageToServer::PlaceBomb: {
            Bomb b;
            b.timer = game_options.bomb_timer;
            b.position = p;
            bombs[static_cast<BombId>(bombs.size())] = b;
            ServerMessageToClient::Event event;
            event.message_type = ServerMessageToClient::Event::BombPlaced;
            ServerMessageToClient::Event::BombPlacedMessage bomb;
            bomb.id = static_cast<BombId>(bombs.size()) - 1;
            bomb.position = p;
            event.message_arguments = bomb;
            events.push_back(event);
            break;
        } default: {
            break;
        }
    }

}

void ServerGame::check_players() {

    for (auto &[key, value] : players) {
        if (killed_robots.contains(key)) {
            // If player was killed we randomize new position for him
            // and add PlayerMovedMessage to events.
            Position p = randomize_position();
            ServerMessageToClient::Event event;
            event.message_type = ServerMessageToClient::Event::PlayerMoved;
            ServerMessageToClient::Event::PlayerMovedMessage moved;
            moved.id = key;
            moved.position = p;
            event.message_arguments = moved;
            events.push_back(event);
            player_positions[key] = p;
            // We also have to update scores for this player.
            scores.find(key)->second += 1;
        } else {
            // If player was not killed and made some move, we parse it.
            if (client_messages.contains(key))
                make_players_move(client_messages.find(key)->second, key);
        }
    }

}

void ServerGame::turn_handler()  {

    check_bombs();
    check_players();
    // We first create TurnMessage, then save it to turn_messages and lastly
    // we send this message to all clients.
    ServerMessageToClient server_message;
    server_message.message_type = ServerMessageToClient::Turn;
    ServerMessageToClient::TurnMessage turn_message;
    turn_message.turn = turn;
    turn_message.events = events;
    server_message.message_arguments = turn_message;
    turn_messages.push_back(server_message);
    for (auto &[key, value] : clients) {
        value->send_message(server_message);
    }
    // Clearing attributes for next turn.
    clear_turn();

    // Checking if game should end.
    if (turn == game_options.game_length + 2) {
        // Sending GameEndedMessage.
        ServerMessageToClient server_message_ended;
        server_message_ended.message_type = ServerMessageToClient::GameEnded;
        ServerMessageToClient::GameEndedMessage game_ended;
        game_ended.scores = scores;
        server_message_ended.message_arguments = game_ended;
        for (auto &[key, value] : clients) {
            value->send_message(server_message_ended);
        }
        // Clearing attributes for next game.
        clear_game();
    } else {
        play_game();
    }

}

void ServerGame::clear_turn() {

    turn++;
    exploded_bombs.clear();
    explosions.clear();
    killed_robots.clear();
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
    client_to_player.clear();

}

void ServerGame::accept_new_player(tcp::socket socket) {

    // Inserts a unique pointer to new client in a map of clients. ClientId is
    // the next available number.
    ClientId number_of_clients = static_cast<ClientId>(clients.size());
    clients[number_of_clients] =
            make_unique<ServerCommunication>(std::move(socket), this, buffer, number_of_clients);

    // Preparing HelloMessage for new client.
    ServerMessageToClient server_message;
    server_message.message_type = ServerMessageToClient::Hello;
    ServerMessageToClient::HelloMessage hello;
    hello.server_name = game_options.server_name;
    hello.players_count = game_options.players_count;
    hello.size_x = game_options.size_x;
    hello.size_y = game_options.size_y;
    hello.game_length = game_options.game_length;
    hello.explosion_radius = game_options.explosion_radius;
    hello.bomb_timer = game_options.bomb_timer;
    server_message.message_arguments = hello;
    // Sending HelloMessage.
    clients.find(number_of_clients)->second->send_message(server_message);

    // Depending on is_lobby state we either send all AcceptedPlayerMessages or
    // GameStartedMessage and TurnMessages.
    if (is_lobby) {
        for (auto &a : accepted_players)
            clients.find(number_of_clients)->second->send_message(a);
    } else {
        clients.find(number_of_clients)->second->send_message(game_started);
        for (auto &t : turn_messages)
            clients.find(number_of_clients)->second->send_message(t);
    }
    clients.find(number_of_clients)->second->receive_message();

}

void ServerGame::check_game_to_start() {

    if (players.size() == game_options.players_count) {
        is_lobby = false;
        start_game();
    }

}

std::string ServerGame::get_player_address(ClientId client_id) {

    return clients.find(client_id)->second->get_client_address();

}

void ServerGame::join_player(ClientMessageToServer &client_message, ClientId client_id) {

    // Adding player to players and client_to_player maps.
    Player player;
    player.name = get<string>(client_message.message_arguments);
    player.address = get_player_address(client_id);
    PlayerId player_id = static_cast<PlayerId>(players.size());
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
    for (auto &[key, value] : clients) {
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

// Removes client from clients map but still holds the client's robot position
// if client was a player.
void ServerGame::disconnect_client(ClientId client_id) {

    clients.erase(client_id);

}
