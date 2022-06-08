#include <boost/program_options.hpp>
#include <arpa/inet.h>
#include "buffer.h"
#include "client_communication.h"
#include "client.h"

using namespace std;
namespace po = boost::program_options;

Client::Client() = default;

void Client::parse_program_options(int argc, char *argv[]) {

    try {
        po::options_description desc("Program usage");
        // Adding all possible options for client program options;
        desc.add_options()
                ("help,h", "produce help message")
                ("gui-address,d", po::value<string>(&gui_address)->required()->
                value_name("<(nazwa hosta):(port) lub (IPv4):(port) lub (IPv6):(port)>"))
                ("player-name,n", po::value<string>(&player_name)->required()->
                value_name("<String>"))
                ("port,p", po::value<uint16_t>(&port)->required()->
                value_name("<u16>"))
                ("server-address,s", po::value<string>(&server_address)->required()->
                value_name("<(nazwa hosta):(port) lub (IPv4):(port) lub (IPv6):(port)>"))
                ;

        const po::positional_options_description p;
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);

        // If there was an option help we don't require other options
        // (this is checked below with notify function).
        if (vm.count("help")) {
            if (debug)
                cout << desc << "\n";
            exit(0);
        }

        po::notify(vm);
    } catch(exception &e) {
        if (debug)
            cerr << "Error: " << e.what() << "\n";
        exit(1);
    } catch(...) {
        if (debug)
            cerr << "Exception of unknown type.\n";
        exit(1);
    }

}

int main(int argc, char* argv[]) {
    Client client;
    client.parse_program_options(argc, argv);

    // Dividing parameters to address and port for both gui and server connection.
    string delimiter = ":";
    string client_gui_address = client.get_gui_address();
    size_t index_delimiter = client_gui_address.rfind(delimiter);
    string gui_host = client_gui_address.substr(0, index_delimiter);
    string gui_port = client_gui_address.substr(index_delimiter + 1, client_gui_address.size());

    string client_server_address = client.get_server_address();
    index_delimiter = client_server_address.rfind(delimiter);
    string server_host = client_server_address.substr(0, index_delimiter);
    string server_port = client_server_address.substr(index_delimiter + 1, client_server_address.size());

    Buffer buffer;
    ClientGame game;
    boost::asio::io_context io_context;
    try {
        ClientCommunication communication(buffer, game, io_context, gui_host, gui_port,
                                           client.get_port(), server_host, server_port,
                                           client.get_player_name());
        communication.server_receive_message();
        communication.gui_receive_message();
        io_context.run();
    } catch (exception &e) {
        if (debug)
            cerr << "Problems with connecting: " << e.what() << '\n';
        exit(1);
    }
    return 0;
}
