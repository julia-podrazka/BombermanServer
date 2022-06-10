#include "server_communication.h"
#include "server_game.h"

using namespace std;
using boost::asio::ip::tcp;

ServerCommunication::ServerCommunication(tcp::socket socket, ServerGame *server_game, Buffer &buffer, ClientId client_id)
    : server_game(server_game), buffer(buffer), socket(move(socket)), client_id(client_id) {

    read_buffer.resize(MAX_BUFFER_SIZE);
    read_buffer_size = 0;
    write_buffer.resize(MAX_BUFFER_SIZE);

}

void ServerCommunication::receive_message() {

    socket.async_receive(boost::asio::buffer(read_buffer) + read_buffer_size,
            [this](boost::system::error_code error, size_t receive_length) {
        receive_handler(error, receive_length);
    });

}

void ServerCommunication::receive_handler(const boost::system::error_code &error, size_t receive_length) {

    if (!error) {
        try {
            ClientMessageToServer client_message;
            read_buffer_size += receive_length;
            // TODO usunąć
            cout << "Buffer received: ";
            for (size_t i = 0; i < receive_length; i++)
                printf("%d", read_buffer[i]);
            cout << '\n';
            // The length that was parsed by Buffer class.
            size_t parse_length =
                    buffer.read_client_message_to_server(client_message, read_buffer, read_buffer_size);
            if (parse_length == 0) {
                // TODO should we disconnect?
            }
            // If a message was successfully parsed, the message is removed
            // from buffer.
            for (size_t i = 0; parse_length + i < MAX_BUFFER_SIZE; i++)
                read_buffer[i] = read_buffer[i + parse_length];
            if (read_buffer_size - parse_length > 0)
                read_buffer_size -= parse_length;
            else
                read_buffer_size = 0;
            server_game->process_client_message(client_message, client_id);
            receive_message();
        } catch (const char *msg) {
            // We have two types of errors: one that means that there is not enough
            // message to parse and we should wait to receive its ending and the
            // second one that means the message from client is not syntax correct
            // and we should disconnect client.
            if (strcmp(msg, "Message not long enough") == 0)
                receive_message();
            else {
                // TODO disconnect client
            }
        }
    } else if (error == boost::asio::error::eof) {
        // If we get eof, client disconnected.
        server_game->disconnect_client(client_id);
    }

}

void ServerCommunication::send_message(ServerMessageToClient &server_message) {

    size_t message_len = 0;
    fill(write_buffer.begin(), write_buffer.end(), 0);
    buffer.write_server_message_to_client(server_message, write_buffer, &message_len);
    // TODO usunąć
    cout << "Buffer to send: ";
    for (size_t i = 0; i < message_len; i++)
        printf("%d", write_buffer[i]);
    cout << '\n';
    socket.async_send(boost::asio::buffer(write_buffer, message_len),
            [this](boost::system::error_code error, size_t send_length) {
        if (error) {
            // TODO what to do?
        }
        if (send_length == 0) {
            // TODO what to do?
        }
    });

}
