#include "client_game.h"

using namespace std;

ClientGame::ClientGame() = default;

bool ClientGame::get_is_lobby() const {

    return is_lobby;

}

void ClientGame::get_hello_message(const ServerMessageToClient::HelloMessage &hello_message) {

    server_name = hello_message.server_name;
    players_count = hello_message.players_count;
    size_x = hello_message.size_x;
    size_y = hello_message.size_y;
    game_length = hello_message.game_length;
    explosion_radius = hello_message.explosion_radius;
    bomb_timer = hello_message.bomb_timer;
    is_lobby = true;

}

void ClientGame::change_game_status_to_started(map<PlayerId, Player> &all_players) {

    is_lobby = false;
    players = all_players;
    for (const auto &player : players)
        scores.insert(pair<PlayerId, Score>(player.first, 0));

}

void ClientGame::change_game_status_to_ended(ClientMessageToGUI &client_message) {

    is_lobby = true;
    prepare_lobby_message(client_message);

    players.clear();
    player_positions.clear();
    blocks.clear();
    bombs.clear();
    explosions.clear();
    scores.clear();

}

void ClientGame::prepare_lobby_message(ClientMessageToGUI &client_message) {

    client_message.message_type = ClientMessageToGUI::Lobby;
    ClientMessageToGUI::LobbyMessage lobby_message;
    lobby_message.server_name = server_name;
    lobby_message.players_count = players_count;
    lobby_message.size_x = size_x;
    lobby_message.size_y = size_y;
    lobby_message.game_length = game_length;
    lobby_message.explosion_radius = explosion_radius;
    lobby_message.bomb_timer = bomb_timer;
    lobby_message.players = players;
    client_message.message_arguments = lobby_message;

}

vector<Position> ClientGame::copy_set_to_vector(const set<pair<CoordinateSize, CoordinateSize>> &set_to_copy) {

    vector<Position> vector_to_copy;
    Position position;
    for (const auto &elem : set_to_copy) {
        position.x = elem.first;
        position.y = elem.second;
        vector_to_copy.push_back(position);
    }
    return vector_to_copy;

}

vector<Bomb> ClientGame::copy_map_to_vector(const map<BombId, Bomb> &bombs_to_copy) {

    vector<Bomb> vector_bombs;
    for (const auto &bomb : bombs_to_copy)
        vector_bombs.push_back(bomb.second);
    return vector_bombs;

}

void ClientGame::prepare_game_message(ClientMessageToGUI &client_message) {

    client_message.message_type = ClientMessageToGUI::Game;
    ClientMessageToGUI::GameMessage game_message;
    game_message.server_name = server_name;
    game_message.size_x = size_x;
    game_message.size_y = size_y;
    game_message.game_length = game_length;
    game_message.turn = turn;
    game_message.players = players;
    game_message.player_positions = player_positions;
    game_message.blocks = copy_set_to_vector(blocks);
    game_message.bombs = copy_map_to_vector(bombs);
    game_message.explosions = copy_set_to_vector(explosions);
    game_message.scores = scores;
    client_message.message_arguments = game_message;

}

void ClientGame::prepare_gui_message(ClientMessageToGUI &client_message) {

    if (is_lobby)
        prepare_lobby_message(client_message);
    else
        prepare_game_message(client_message);

}

void ClientGame::place_bomb(ServerMessageToClient::Event::BombPlacedMessage &bomb_placed) {

    Bomb new_bomb;
    new_bomb.position = bomb_placed.position;
    new_bomb.timer = bomb_timer;
    bombs.insert(pair<BombId, Bomb>(bomb_placed.id, new_bomb));

}

void ClientGame::explosion_from_bomb(ServerMessageToClient::Event::BombExplodedMessage &bomb_exploded) {

    Position bomb_position = bombs[bomb_exploded.id].position;
    explosions.insert(pair<CoordinateSize, CoordinateSize>(bomb_position.x, bomb_position.y));
    // Adds explosions from above, below, on the right/left to the exploding bomb depending
    // on explosion_radius and whether there is a block blocking the way.
    if (blocks.count(pair<CoordinateSize, CoordinateSize>(bomb_position.x, bomb_position.y)) == 0) {
        if (bomb_position.y != size_y - 1) {
            for (CoordinateSize i = bomb_position.y + 1, k = 0; (k < explosion_radius && i < size_y); i++, k++) {
                if (blocks.count(pair<CoordinateSize, CoordinateSize>(bomb_position.x, i)) == 0)
                    explosions.insert(pair<CoordinateSize, CoordinateSize>(bomb_position.x, i));
                else
                    break;
            }
        }
        if (bomb_position.x != size_x - 1) {
            for (CoordinateSize i = bomb_position.x + 1, k = 0; (k < explosion_radius && i < size_x); i++, k++) {
                if (blocks.count(pair<CoordinateSize, CoordinateSize>(i, bomb_position.y)) == 0)
                    explosions.insert(pair<CoordinateSize, CoordinateSize>(i, bomb_position.y));
                else
                    break;
            }
        }
        if (bomb_position.y != 0) {
            for (CoordinateSize i = bomb_position.y - 1, k = 0; k < explosion_radius; i--, k++) {
                if (blocks.count(pair<CoordinateSize, CoordinateSize>(bomb_position.x, i)) == 0)
                    explosions.insert(pair<CoordinateSize, CoordinateSize>(bomb_position.x, i));
                else
                    break;
                if (i == 0)
                    break;
            }
        }
        if (bomb_position.x != 0) {
            for (CoordinateSize i = bomb_position.x - 1, k = 0; k < explosion_radius; i--, k++) {
                if (blocks.count(pair<CoordinateSize, CoordinateSize>(i, bomb_position.y)) == 0)
                    explosions.insert(pair<CoordinateSize, CoordinateSize>(i, bomb_position.y));
                else
                    break;
                if (i == 0)
                    break;
            }
        }
    }
    bombs.erase(bomb_exploded.id);

}

void ClientGame::explode_bomb(ServerMessageToClient::Event::BombExplodedMessage &bomb_exploded) {

    Position player_position;
    Score player_score;
    // For every robot killed (if this is their first kill this turn) we increase their score
    // and insert their position into explosions set.
    for (const auto &robot_destroyed : bomb_exploded.robots_destroyed) {
        if (killed_robots.count(robot_destroyed) == 0) {
            player_score = scores[robot_destroyed];
            scores.erase(robot_destroyed);
            scores.insert(pair<PlayerId, Score>(robot_destroyed, (player_score + 1)));
            killed_robots.insert(robot_destroyed);
            player_position = player_positions[robot_destroyed];
            explosions.insert(pair<CoordinateSize, CoordinateSize>(player_position.x, player_position.y));
            player_positions.erase(robot_destroyed);
        }
    }
    explosion_from_bomb(bomb_exploded);
    // For every block we insert its position into explosions set and insert its position into
    // blocks_to_destroy to destroy it later.
    for (const auto &block_destroyed : bomb_exploded.blocks_destroyed) {
        explosions.insert(pair<CoordinateSize, CoordinateSize>(block_destroyed.x, block_destroyed.y));
        blocks_to_destroy.insert(pair<CoordinateSize, CoordinateSize>(block_destroyed.x, block_destroyed.y));
    }

}

void ClientGame::move_player(ServerMessageToClient::Event::PlayerMovedMessage &player_moved) {

    player_positions.erase(player_moved.id);
    player_positions.insert(pair<PlayerId, Position>(player_moved.id, player_moved.position));

}

void ClientGame::place_block(ServerMessageToClient::Event::BlockPlacedMessage &block_placed) {

    blocks.insert(pair<CoordinateSize, CoordinateSize>(block_placed.position.x, block_placed.position.y));

}

void ClientGame::get_turn_message(ServerMessageToClient::TurnMessage &turn_message) {

    turn = turn_message.turn;

    for (auto &bomb : bombs)
        bomb.second.timer -= 1;

    killed_robots.clear();
    explosions.clear();
    blocks_to_destroy.clear();

    // We parse every event separately.
    for (auto event : turn_message.events) {
        switch (event.message_type) {
            case ServerMessageToClient::Event::BombPlaced:
                place_bomb(get<ServerMessageToClient::Event::BombPlacedMessage>(event.message_arguments));
                break;
            case ServerMessageToClient::Event::BombExploded:
                explode_bomb(get<ServerMessageToClient::Event::BombExplodedMessage>(event.message_arguments));
                break;
            case ServerMessageToClient::Event::PlayerMoved:
                move_player(get<ServerMessageToClient::Event::PlayerMovedMessage>(event.message_arguments));
                break;
            case ServerMessageToClient::Event::BlockPlaced:
                place_block(get<ServerMessageToClient::Event::BlockPlacedMessage>(event.message_arguments));
                break;
        }
    }
    // At the end of the events blocks from bombs are destroyed.
    for (const auto &block_destroyed : blocks_to_destroy)
        blocks.erase(pair<CoordinateSize, CoordinateSize>(block_destroyed.first, block_destroyed.second));

}

void ClientGame::accept_player(const ServerMessageToClient::AcceptedPlayerMessage &accepted_player) {

    players.insert(pair<PlayerId, Player>(accepted_player.id, accepted_player.player));

}
