#ifndef BUFFER_H
#define BUFFER_H

#include <string>
#include <cstddef>
#include "messages.h"

#define MAX_BUFFER_SIZE 65535

class Buffer {

    // Functions write_... write a message to a buffer_send from a structure
    // passed as a parameter.

    // Functions read_... read a message from a buffer passed a parameter
    // and save it into a structure (also passed as parameter).
    // If we read a message from tcp server, we check whether the whole message
    // has been read already.

private:

    std::vector<uint8_t> buffer_send;
    size_t buffer_index_send; // place in a buffer_send where next information should be written
    size_t buffer_index_read_udp;
    size_t buffer_index_read_tcp;

    void clean_send_buffer();

    void write_lobby_message(const ClientMessageToGUI::LobbyMessage &lobby_message);

    void write_game_message(const ClientMessageToGUI::GameMessage &game_message);

    void read_hello_message(ServerMessageToClient::HelloMessage &server_message, std::vector<uint8_t> &buffer, size_t len);

    void read_accepted_player_message(ServerMessageToClient::AcceptedPlayerMessage &server_message, std::vector<uint8_t> &buffer, size_t len);

    void read_game_started_message(ServerMessageToClient::GameStartedMessage &server_message, std::vector<uint8_t> &buffer, size_t len);

    void read_position_message(Position &position, std::vector<uint8_t> &buffer);

    void read_event_message(ServerMessageToClient::Event &event_message, std::vector<uint8_t> &buffer, size_t len);

    void read_turn_message(ServerMessageToClient::TurnMessage &server_message, std::vector<uint8_t> &buffer, size_t len);

    void read_game_ended_message(ServerMessageToClient::GameEndedMessage &server_message, std::vector<uint8_t> &buffer, size_t len);

public:
    Buffer();

    // Converts number from host to network.
    template<typename T>
    T convert_to_network(T number);

    // Converts number from network to host.
    template<typename T>
    T convert_from_network(T number);

    template<typename T>
    void write_number_to_buffer(T number);

    void write_string_to_buffer(const std::string& word);

    std::vector<uint8_t> &write_client_message_to_server(const ClientMessageToServer &client_message, size_t *len);

    std::vector<uint8_t> &write_client_message_to_gui(const ClientMessageToGUI &client_message, size_t *len);

    template<typename T>
    void read_number_from_buffer(T &number, std::vector<uint8_t> &buffer, size_t *buffer_index);

    std::string read_string_from_buffer(std::vector<uint8_t> &buffer, size_t len);

    bool read_gui_message_to_client(GUIMessageToClient &gui_message, std::vector<uint8_t> &buffer, size_t len);

    size_t read_server_message_to_client(ServerMessageToClient &server_message, std::vector<uint8_t> &buffer, size_t len);

};


#endif