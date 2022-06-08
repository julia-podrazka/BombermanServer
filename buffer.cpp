#include <iostream>
#include <arpa/inet.h>
#include <cstring>
#include "buffer.h"

#define UIN32_T_BYTES 4
#define UIN16_T_BYTES 2
#define UIN8_T_BYTES 1
#define SERVER_ENUMS 4
#define GUI_ENUMS 2

using namespace std;

void Buffer::clean_send_buffer() {

    buffer_index_send = 0;
    fill(buffer_send.begin(), buffer_send.end(), 0);

}

Buffer::Buffer() {

    buffer_send.resize(MAX_BUFFER_SIZE);
    clean_send_buffer();
    buffer_index_read_tcp = 0;
    buffer_index_read_udp = 0;

}

template<> uint8_t Buffer::convert_to_network<uint8_t>(uint8_t number) {

    return number;

}

template<> uint16_t Buffer::convert_to_network<uint16_t>(uint16_t number) {

    return htons(number);

}

template<> uint32_t Buffer::convert_to_network<uint32_t>(uint32_t number) {

    return htonl(number);

}

template<> uint8_t Buffer::convert_from_network<uint8_t>(uint8_t number) {

    return number;

}

template<> uint16_t Buffer::convert_from_network<uint16_t>(uint16_t number) {

    return ntohs(number);

}

template<> uint32_t Buffer::convert_from_network<uint32_t>(uint32_t number) {

    return ntohl(number);

}

// Converting number from host to network and writing it in a buffer at a
// right index.
template<typename T>
void Buffer::write_number_to_buffer(T number) {

    *(T *)(&buffer_send[buffer_index_send]) = convert_to_network(number);
    buffer_index_send += sizeof(T);

}

void Buffer::write_string_to_buffer(const string& word) {

    write_number_to_buffer(uint8_t(word.length()));
    memcpy((&buffer_send[buffer_index_send]), word.c_str(), word.length());
    buffer_index_send += word.length();

}

vector<uint8_t> &Buffer::write_client_message_to_server(const ClientMessageToServer &client_message, size_t *len) {

    clean_send_buffer();
    ClientMessageToServer::MessageType type = client_message.message_type;

    write_number_to_buffer(uint8_t(type));
    if (type == ClientMessageToServer::Join)
        write_string_to_buffer(get<string>(client_message.message_arguments));
    else if (type == ClientMessageToServer::Move)
        write_number_to_buffer(uint8_t(get<Direction>(client_message.message_arguments)));

    (*len) = buffer_index_send;
    return buffer_send;

}

void Buffer::write_lobby_message(const ClientMessageToGUI::LobbyMessage &lobby_message) {

    write_string_to_buffer(lobby_message.server_name);
    write_number_to_buffer(lobby_message.players_count);
    write_number_to_buffer(lobby_message.size_x);
    write_number_to_buffer(lobby_message.size_y);
    write_number_to_buffer(lobby_message.game_length);
    write_number_to_buffer(lobby_message.explosion_radius);
    write_number_to_buffer(lobby_message.bomb_timer);
    write_number_to_buffer(uint32_t(lobby_message.players.size()));
    for (const auto& [key, value] : lobby_message.players) {
        write_number_to_buffer(key);
        write_string_to_buffer(value.name);
        write_string_to_buffer(value.address);
    }

}

void Buffer::write_game_message(const ClientMessageToGUI::GameMessage &game_message) {

    write_string_to_buffer(game_message.server_name);
    write_number_to_buffer(game_message.size_x);
    write_number_to_buffer(game_message.size_y);
    write_number_to_buffer(game_message.game_length);
    write_number_to_buffer(game_message.turn);
    write_number_to_buffer(uint32_t(game_message.players.size()));
    for (const auto& [key, value] : game_message.players) {
        write_number_to_buffer(key);
        write_string_to_buffer(value.name);
        write_string_to_buffer(value.address);
    }
    write_number_to_buffer(uint32_t(game_message.player_positions.size()));
    for (const auto& [key, value] : game_message.player_positions) {
        write_number_to_buffer(key);
        write_number_to_buffer(value.x);
        write_number_to_buffer(value.y);
    }
    write_number_to_buffer(uint32_t(game_message.blocks.size()));
    for (Position p : game_message.blocks) {
        write_number_to_buffer(p.x);
        write_number_to_buffer(p.y);
    }
    write_number_to_buffer(uint32_t(game_message.bombs.size()));
    for (Bomb b : game_message.bombs) {
        write_number_to_buffer(b.position.x);
        write_number_to_buffer(b.position.y);
        write_number_to_buffer(b.timer);
    }
    write_number_to_buffer(uint32_t(game_message.explosions.size()));
    for (Position p : game_message.explosions) {
        write_number_to_buffer(p.x);
        write_number_to_buffer(p.y);
    }
    write_number_to_buffer(uint32_t(game_message.scores.size()));
    for (const auto& [key, value] : game_message.scores) {
        write_number_to_buffer(key);
        write_number_to_buffer(value);
    }

}

vector<uint8_t> &Buffer::write_client_message_to_gui(const ClientMessageToGUI &client_message, size_t *len) {

    clean_send_buffer();
    ClientMessageToGUI::DrawMessage type = client_message.message_type;

    write_number_to_buffer(uint8_t(type));
    if (type == ClientMessageToGUI::Lobby)
        write_lobby_message(get<ClientMessageToGUI::LobbyMessage>(client_message.message_arguments));
    else
        write_game_message(get<ClientMessageToGUI::GameMessage>(client_message.message_arguments));

    (*len) = buffer_index_send;
    return buffer_send;

}

template<typename T>
void Buffer::read_number_from_buffer(T &number, vector<uint8_t> &buffer, size_t *buffer_index) {

    number = convert_from_network(*(T *)(&buffer[(*buffer_index)]));
    (*buffer_index) += sizeof(T);

}

string Buffer::read_string_from_buffer(vector<uint8_t> &buffer, size_t len) {

    // If there is not enough message to be read we throw an exception.
    if (len < buffer_index_read_tcp + UIN8_T_BYTES)
        throw "Message not long enough";
    uint8_t string_length = convert_from_network(*(uint8_t *)(&buffer[buffer_index_read_tcp]));
    buffer_index_read_tcp++;
    if (len < buffer_index_read_tcp + string_length)
        throw "Message not long enough";
    size_t old_index = buffer_index_read_tcp;
    buffer_index_read_tcp += string_length;
    return string((begin(buffer) + old_index), (begin(buffer) + buffer_index_read_tcp));

}

bool Buffer::read_gui_message_to_client(GUIMessageToClient &gui_message, vector<uint8_t> &buffer, size_t len) {

    if (len < UIN8_T_BYTES)
        return false;
    buffer_index_read_udp = 0;
    uint8_t message_enum;
    read_number_from_buffer(message_enum, buffer, &buffer_index_read_udp);
    if (message_enum > GUI_ENUMS)
        return false;
    gui_message.message_type = static_cast<GUIMessageToClient::InputMessage>(message_enum);
    if (gui_message.message_type == GUIMessageToClient::Move) {
        if (len != (2 * UIN8_T_BYTES))
            return false;
        read_number_from_buffer(message_enum, buffer, &buffer_index_read_udp);
        gui_message.message_arguments = static_cast<Direction>(message_enum);
    }
    return true;

}

void Buffer::read_hello_message(ServerMessageToClient::HelloMessage &server_message, vector<uint8_t> &buffer, size_t len) {

    try {
        server_message.server_name = read_string_from_buffer(buffer, len);
    } catch (const char *msg) {
        throw msg;
    }
    if (len < buffer_index_read_tcp + (3 * UIN8_T_BYTES + UIN32_T_BYTES + 2 * UIN16_T_BYTES))
        throw "Message not long enough";
    read_number_from_buffer(server_message.players_count, buffer, &buffer_index_read_tcp);
    read_number_from_buffer(server_message.size_x, buffer, &buffer_index_read_tcp);
    read_number_from_buffer(server_message.size_y, buffer, &buffer_index_read_tcp);
    read_number_from_buffer(server_message.game_length, buffer, &buffer_index_read_tcp);
    read_number_from_buffer(server_message.explosion_radius, buffer, &buffer_index_read_tcp);
    read_number_from_buffer(server_message.bomb_timer, buffer, &buffer_index_read_tcp);

}

void Buffer::read_accepted_player_message(ServerMessageToClient::AcceptedPlayerMessage &server_message, vector<uint8_t> &buffer, size_t len) {

    if (len < buffer_index_read_tcp + UIN32_T_BYTES)
        throw "Message not long enough";
    read_number_from_buffer(server_message.id, buffer, &buffer_index_read_tcp);
    try {
        server_message.player.name = read_string_from_buffer(buffer, len);
        server_message.player.address = read_string_from_buffer(buffer, len);
    } catch (const char *msg) {
        throw msg;
    }

}

void Buffer::read_game_started_message(ServerMessageToClient::GameStartedMessage &server_message, vector<uint8_t> &buffer, size_t len) {

    uint32_t map_length;
    PlayerId key;
    Player value;
    if (len < buffer_index_read_tcp + UIN32_T_BYTES)
        throw "Message not long enough";
    read_number_from_buffer(map_length, buffer, &buffer_index_read_tcp);
    for (uint32_t i = 0; i < map_length; i++) {
        if (len < buffer_index_read_tcp + UIN8_T_BYTES)
            throw "Message not long enough";
        read_number_from_buffer(key, buffer, &buffer_index_read_tcp);
        try {
            value.name = read_string_from_buffer(buffer, len);
            value.address = read_string_from_buffer(buffer, len);
            server_message.players.insert(pair<PlayerId, Player>(key, value));
        } catch (const char *msg) {
            throw msg;
        }
    }

}

void Buffer::read_position_message(Position &position, vector<uint8_t> &buffer) {

    read_number_from_buffer(position.x, buffer, &buffer_index_read_tcp);
    read_number_from_buffer(position.y, buffer, &buffer_index_read_tcp);

};

void Buffer::read_event_message(ServerMessageToClient::Event &event_message, vector<uint8_t> &buffer, size_t len) {

    if (len < buffer_index_read_tcp + UIN8_T_BYTES)
        throw "Message not long enough";
    uint8_t message_enum;
    read_number_from_buffer(message_enum, buffer, &buffer_index_read_tcp);
    event_message.message_type = static_cast<ServerMessageToClient::Event::EventMessage>(message_enum);

    // We parse events depending on its type.
    switch (event_message.message_type) {
        case ServerMessageToClient::Event::BombPlaced: {
            ServerMessageToClient::Event::BombPlacedMessage message;
            if (len < buffer_index_read_tcp + (UIN32_T_BYTES + 2 * UIN16_T_BYTES))
                throw "Message not long enough";
            read_number_from_buffer(message.id, buffer, &buffer_index_read_tcp);
            read_position_message(message.position, buffer);
            event_message.message_arguments = message;
            break;
        } case ServerMessageToClient::Event::BombExploded: {
            ServerMessageToClient::Event::BombExplodedMessage message;
            if (len < buffer_index_read_tcp + (UIN32_T_BYTES + 2 * UIN16_T_BYTES))
                throw "Message not long enough";
            read_number_from_buffer(message.id, buffer, &buffer_index_read_tcp);
            uint32_t vector_length;
            read_number_from_buffer(vector_length, buffer, &buffer_index_read_tcp);
            if (len < buffer_index_read_tcp + vector_length)
                throw "Message not long enough";
            PlayerId id;
            for (uint32_t i = 0; i < vector_length; i++) {
                read_number_from_buffer(id, buffer, &buffer_index_read_tcp);
                message.robots_destroyed.push_back(id);
            }
            if (len < buffer_index_read_tcp + UIN8_T_BYTES)
                throw "Message not long enough";
            read_number_from_buffer(vector_length, buffer, &buffer_index_read_tcp);
            if (len < buffer_index_read_tcp + vector_length * UIN32_T_BYTES)
                throw "Message not long enough";
            Position position;
            for (uint32_t i = 0; i < vector_length; i++) {
                read_position_message(position, buffer);
                message.blocks_destroyed.push_back(position);
            }
            event_message.message_arguments = message;
            break;
        } case ServerMessageToClient::Event::PlayerMoved: {
            ServerMessageToClient::Event::PlayerMovedMessage message;
            if (len < buffer_index_read_tcp + (UIN32_T_BYTES + UIN8_T_BYTES))
                throw "Message not long enough";
            read_number_from_buffer(message.id, buffer, &buffer_index_read_tcp);
            read_position_message(message.position, buffer);
            event_message.message_arguments = message;
            break;
        } case ServerMessageToClient::Event::BlockPlaced: {
            ServerMessageToClient::Event::BlockPlacedMessage message;
            if (len < buffer_index_read_tcp + UIN32_T_BYTES)
                throw "Message not long enough";
            read_position_message(message.position, buffer);
            event_message.message_arguments = message;
            break;
        } default: {
            break;
        }
    }

}

void Buffer::read_turn_message(ServerMessageToClient::TurnMessage &server_message, vector<uint8_t> &buffer, size_t len) {

    if (len < buffer_index_read_tcp + (UIN32_T_BYTES + 2 * UIN8_T_BYTES))
        throw "Message not long enough";
    read_number_from_buffer(server_message.turn, buffer, &buffer_index_read_tcp);
    uint32_t vector_length;
    read_number_from_buffer(vector_length, buffer, &buffer_index_read_tcp);
    try {
        for (uint32_t i = 0; i < vector_length; i++) {
            ServerMessageToClient::Event event;
            read_event_message(event, buffer, len);
            server_message.events.push_back(event);
        }
    } catch (const char *msg) {
        throw msg;
    }

}

void Buffer::read_game_ended_message(ServerMessageToClient::GameEndedMessage &server_message, vector<uint8_t> &buffer, size_t len) {

    if (len < (UIN32_T_BYTES + UIN8_T_BYTES))
        throw "Message not long enough";
    uint32_t map_length;
    PlayerId key;
    Score value;
    read_number_from_buffer(map_length, buffer, &buffer_index_read_tcp);
    // We calculate how much bytes we should have in a buffer, because
    // every element of a map contains uin32_t and uint8_t.
    if (len < (UIN32_T_BYTES + UIN8_T_BYTES) * map_length + buffer_index_read_tcp)
        throw "Message not long enough";
    for (uint32_t i = 0; i < map_length; i++) {
        read_number_from_buffer(key, buffer, &buffer_index_read_tcp);
        read_number_from_buffer(value, buffer, &buffer_index_read_tcp);
        server_message.scores.insert(pair<PlayerId, Score>(key, value));
    }

}

// Throws an exception if there is not enough message from server. If not, the message
// is parsed to server_message.
size_t Buffer::read_server_message_to_client(ServerMessageToClient &server_message, vector<uint8_t> &buffer, size_t len) {

    try {
        // We don't check if message enum is in a vector, because this function
        // is called only when at least 1 byte was send to client from server.
        buffer_index_read_tcp = 0;
        uint8_t message_enum;
        read_number_from_buffer(message_enum, buffer, &buffer_index_read_tcp);
        if (message_enum > SERVER_ENUMS)
            return 0;
        server_message.message_type = static_cast<ServerMessageToClient::MessageType>(message_enum);

        switch (server_message.message_type) {
            case ServerMessageToClient::Hello: {
                ServerMessageToClient::HelloMessage hello_message;
                read_hello_message(hello_message, buffer, len);
                server_message.message_arguments = hello_message;
                break;
            } case ServerMessageToClient::AcceptedPlayer: {
                ServerMessageToClient::AcceptedPlayerMessage accepted_message;
                read_accepted_player_message(accepted_message, buffer, len);
                server_message.message_arguments = accepted_message;
                break;
            } case ServerMessageToClient::GameStarted: {
                ServerMessageToClient::GameStartedMessage started_message;
                read_game_started_message(started_message, buffer, len);
                server_message.message_arguments = started_message;
                break;
            } case ServerMessageToClient::Turn: {
                ServerMessageToClient::TurnMessage turn_message;
                read_turn_message(turn_message, buffer, len);
                server_message.message_arguments = turn_message;
                break;
            } case ServerMessageToClient::GameEnded: {
                ServerMessageToClient::GameEndedMessage ended_message;
                read_game_ended_message(ended_message, buffer, len);
                server_message.message_arguments = ended_message;
                break;
            } default: {
                break;
            }
        }
        return buffer_index_read_tcp;
    } catch (const char *msg) {
        throw msg;
    }

}
