#ifndef SERVER_COMMUNICATION_H
#define SERVER_COMMUNICATION_H

#include <boost/lexical_cast.hpp>
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

    ServerCommunication(boost::asio::ip::tcp::socket socket, ServerGame *server_game, Buffer &buffer, ClientId client_id);

    std::string get_client_address() {

        return boost::lexical_cast<std::string>(socket.remote_endpoint());

    }

    void receive_message();

    void send_message(ServerMessageToClient &server_message);

};

#endif
