#ifndef SIK_ZADANIE_2_COMMUNICATION_H
#define SIK_ZADANIE_2_COMMUNICATION_H

#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include "client_game.h"
#include "buffer.h"

class ClientCommunication {

private:

    Buffer buffer;
    ClientGame game;
    std::string player_name;
    std::vector<uint8_t> gui_buffer;
    std::vector<uint8_t> server_buffer;
    size_t server_buffer_size;
    boost::asio::ip::udp::resolver gui_resolver;
    boost::asio::ip::udp::endpoint gui_endpoints;
    boost::asio::ip::udp::socket gui_socket;
    boost::asio::ip::tcp::resolver server_resolver;
    boost::asio::ip::tcp::resolver::results_type server_endpoints;
    boost::asio::ip::tcp::socket server_socket;

    void prepare_server_message(ClientMessageToServer &client_message, GUIMessageToClient &gui_message);

public:

    ClientCommunication(Buffer &buffer, ClientGame &game, boost::asio::io_context &io_context, std::string &gui_host, std::string &gui_port, uint16_t port, std::string &server_host, std::string &server_port, std::string &player_name);

    ~ClientCommunication();

    void gui_receive_message();

    void gui_handler(const boost::system::error_code &error, size_t receive_length);

    void server_receive_message();

    void server_handler(const boost::system::error_code &error, size_t receive_length);

};

#endif //SIK_ZADANIE_2_COMMUNICATION_H
