#ifndef SERVER_COMMUNICATION_H
#define SERVER_COMMUNICATION_H

#include <boost/asio.hpp>
#include "buffer.h"
#include "messages.h"

// Because PlayerId is only assigned to the clients that sent Join,
// all clients will have their own ClientId, unrelated to PlayerId.
using ClientId = uint8_t;

class ServerGame;

class ServerCommunication {

private:

    ServerGame *server_game;
    Buffer buffer;
    std::vector<uint8_t> read_buffer;
    std::vector<uint8_t> write_buffer;
    size_t read_buffer_size;
    boost::asio::ip::tcp::socket socket;
    ClientId client_id;

    void receive_handler(const boost::system::error_code &error, size_t receive_length);

public:

    ServerCommunication(boost::asio::ip::tcp::socket socket, ServerGame *server_game, Buffer &buffer, size_t client_id);

    std::string get_client_address() {

        return socket.remote_endpoint().address().to_string();

    }

    uint16_t get_client_port() {

        return socket.remote_endpoint().port();

    }

    void receive_message();

    void send_message(ServerMessageToClient &server_message);

};

#endif
