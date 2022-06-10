#include <iostream>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include "server.h"

using namespace std;
namespace po = boost::program_options;
using boost::asio::ip::tcp;

Server::Server() = default;

void Server::parse_program_options(int argc, char *argv[]) {

    // Because of problems with uint8_t, we first save players_count in this
    // help variable and then we cast it to uint8_t.
    unsigned help_players_count;

    try {
        po::options_description desc("Program usage");
        // Adding all possible options for server program options;
        desc.add_options()
                ("help,h", "produce help message")
                ("bomb-timer,b", po::value<uint16_t>(&game_options.bomb_timer)->required()->
                        value_name("<u16>"))
                ("players-count,c", po::value<unsigned>(&help_players_count)->required()->
                        value_name("<u8>"))
                ("turn-duration,d", po::value<uint64_t>(&game_options.turn_duration)->required()->
                        value_name("<u64, milisekundy>"))
                ("explosion-radius,e", po::value<uint16_t>(&game_options.explosion_radius)->required()->
                        value_name("<u16>"))
                ("initial-blocks,k", po::value<uint16_t>(&game_options.initial_blocks)->required()->
                        value_name("<u16>"))
                ("game-length,l", po::value<uint16_t>(&game_options.game_length)->required()->
                        value_name("<u16>"))
                ("server-name,n", po::value<string>(&game_options.server_name)->required()->
                        value_name("<String>"))
                ("port,p", po::value<uint16_t>(&port)->required()->
                        value_name("<u16>"))
                ("seed,s", po::value<uint32_t>(&game_options.seed)->
                        default_value(static_cast<uint32_t>(chrono::system_clock::now().time_since_epoch().count()))->
                        value_name("<u32, parametr opcjonalny>"))
                ("size-x,x", po::value<uint16_t>(&game_options.size_x)->required()->
                        value_name("<u16>"))
                ("size-y,y", po::value<uint16_t>(&game_options.size_y)->required()->
                        value_name("<u16>"))
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

        game_options.players_count = boost::numeric_cast<uint8_t>(help_players_count);

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

void Server::accept_players(tcp::acceptor *acceptor, ServerGame *server_game) {

    // TODO Usunąć
//    try {
//        acceptor->async_accept([this, &acceptor, &server_game](boost::system::error_code ec, tcp::socket socket) {
//            if (!ec) {
//                socket.set_option(tcp::no_delay(true));
//                server_game->accept_new_player(std::move(socket));
//            }
//            accept_players(acceptor, server_game);
//        });
//    } catch (const exception &e) {
//        accept_players(acceptor, server_game);
//    }
    acceptor->async_accept([this, &acceptor, &server_game](boost::system::error_code ec, tcp::socket socket) {
        if (!ec) {
            socket.set_option(tcp::no_delay(true));
            server_game->accept_new_player(std::move(socket));
        }
        accept_players(acceptor, server_game);
    });

}

int main(int argc, char* argv[]) {

    Buffer buffer;

    // Parse program options.
    Server server;
    server.parse_program_options(argc, argv);

    // Accept players and run io_context.
    try {
        boost::asio::io_context io_context;
        tcp::endpoint endpoint(tcp::v6(), server.get_port());
        tcp::acceptor acceptor(io_context, endpoint);
        // Create ServerGame that will hold the state of the game.
        ServerGame game(server.get_game_options(), buffer, io_context);
        server.accept_players(&acceptor, &game);
        io_context.run();
    } catch (const exception &e) {
        if (debug)
            cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}
