#include "client_communication.h"

using namespace std;
using boost::asio::ip::udp;
using boost::asio::ip::tcp;

// Constructor establishes the connection with tcp server and udp gui.
ClientCommunication::ClientCommunication(Buffer &buffer, ClientGame &game, boost::asio::io_context &io_context, string &gui_host, string &gui_port, uint16_t port,
        string &server_host, string &server_port, string &player_name)
        : buffer(buffer), game(game), player_name(player_name), gui_resolver(io_context), gui_endpoints(*gui_resolver.resolve(gui_host, gui_port)),
        gui_socket(io_context, udp::endpoint(udp::v6(), port)),
        server_resolver(io_context), server_endpoints(server_resolver.resolve(server_host, server_port)), server_socket(io_context) {

    gui_buffer.resize(MAX_BUFFER_SIZE);
    server_buffer.resize(MAX_BUFFER_SIZE);
    server_buffer_size = 0;
    boost::asio::connect(server_socket, server_endpoints);
    server_socket.set_option(tcp::no_delay(true));

}

ClientCommunication::~ClientCommunication() {

    server_socket.close();
    gui_socket.close();

}

void ClientCommunication::prepare_server_message(ClientMessageToServer &client_message,
                                              GUIMessageToClient &gui_message) {

    if (game.get_is_lobby()) {
        client_message.message_type = ClientMessageToServer::Join;
        client_message.message_arguments = player_name;
    } else {
        switch (gui_message.message_type) {
            case GUIMessageToClient::PlaceBlock: {
                client_message.message_type = ClientMessageToServer::PlaceBlock;
                break;
            } case GUIMessageToClient::PlaceBomb: {
                client_message.message_type = ClientMessageToServer::PlaceBomb;
                break;
            } case GUIMessageToClient::Move: {
                client_message.message_type = ClientMessageToServer::Move;
                client_message.message_arguments = gui_message.message_arguments;
                break;
            }
        }
    }

}

void ClientCommunication::gui_receive_message() {

    gui_socket.async_receive_from(boost::asio::buffer(gui_buffer, MAX_BUFFER_SIZE), gui_endpoints, [this](boost::system::error_code error, size_t receive_length) {
        gui_handler(error, receive_length);
    });

}

void ClientCommunication::gui_handler(const boost::system::error_code &error, size_t receive_length) {

    if (!error) {
        GUIMessageToClient gui_message_to_client;
        if (!buffer.read_gui_message_to_client(gui_message_to_client, gui_buffer, receive_length)) {
            gui_receive_message();
        } else {
            ClientMessageToServer client_message_to_server;
            prepare_server_message(client_message_to_server, gui_message_to_client);
            size_t len;
            vector<uint8_t> send_vector = buffer.write_client_message_to_server(client_message_to_server, &len);
            server_socket.async_send(boost::asio::buffer(send_vector, len), [this](boost::system::error_code error, size_t send_length) {
                if (!error) {
                    if (send_length == 0) {
                        if (debug) cerr << "Server ended connection with client.";
                        exit(1);
                    }
                    gui_receive_message();
                } else {
                    if (debug) cerr << "Error sending message to server: " << error << '\n';
                    exit(1);
                }
            });
        }
    } else {
        if (debug) cerr << "Error reading from gui: " << error << '\n';
        exit(1);
    }

}

void ClientCommunication::server_receive_message() {

    server_socket.async_receive(boost::asio::buffer(server_buffer) + server_buffer_size,
            [this](boost::system::error_code error, size_t receive_length) {
        server_handler(error, receive_length);
    });

}

void ClientCommunication::server_handler(const boost::system::error_code &error,
                                      size_t receive_length) {

    if (!error) {
        try {
            ServerMessageToClient server_message_to_client;
            server_buffer_size += receive_length;
            size_t parse_length =
                    buffer.read_server_message_to_client(server_message_to_client, server_buffer, server_buffer_size);
            if (parse_length == 0) {
                if (debug) cout << "Incorrect message from server.\n";
                exit(1);
            }
            for (size_t i = 0; parse_length + i < MAX_BUFFER_SIZE; i++)
                server_buffer[i] = server_buffer[i + parse_length];
            if (server_buffer_size - parse_length > 0)
                server_buffer_size -= parse_length;
            else
                server_buffer_size = 0;
            // We prepare a message from client to gui.
            ClientMessageToGUI client_message_to_gui;
            switch (server_message_to_client.message_type) {
                case ServerMessageToClient::Hello: {
                    game.get_hello_message(get<ServerMessageToClient::HelloMessage>(server_message_to_client.message_arguments));
                    game.prepare_gui_message(client_message_to_gui);
                    break;
                } case ServerMessageToClient::GameStarted: {
                    game.change_game_status_to_started(get<ServerMessageToClient::GameStartedMessage>(server_message_to_client.message_arguments).players);
                    break;
                } case ServerMessageToClient::GameEnded: {
                    game.change_game_status_to_ended(client_message_to_gui);
                    break;
                } case ServerMessageToClient::AcceptedPlayer: {
                    game.accept_player(get<ServerMessageToClient::AcceptedPlayerMessage>(server_message_to_client.message_arguments));
                    game.prepare_gui_message(client_message_to_gui);
                    break;
                } default: {
                    game.get_turn_message(get<ServerMessageToClient::TurnMessage>(server_message_to_client.message_arguments));
                    game.prepare_gui_message(client_message_to_gui);
                    break;
                }
            }
            // We don't send anything back after GameStarted message.
            if (server_message_to_client.message_type == ServerMessageToClient::GameStarted) {
                server_receive_message();
            } else {
                size_t len;
                vector<uint8_t> send_vector = buffer.write_client_message_to_gui(client_message_to_gui, &len);
                gui_socket.async_send_to(boost::asio::buffer(send_vector, len), gui_endpoints,
                        [this](boost::system::error_code error, size_t send_length) {
                    if (send_length == 0) {
                        if (debug) cerr << "Connection closed with GUI.\n";
                        exit(1);
                    }
                    if (!error) {
                        server_receive_message();
                    } else {
                        if (debug) cerr << "Error sending message to server: " << error << '\n';
                        exit(1);
                    }
                });
            }
        } catch (const char *msg) {
            server_receive_message();
        }
    } else if (error == boost::asio::error::eof || receive_length == 0) {
        if (debug) cerr << "Server ended connection with client.";
        exit(1);
    } else {
        if (debug) cerr << "Error reading from gui: " << error << '\n';
        exit(1);
    }

}
