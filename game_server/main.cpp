#include "server.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) { // Expecting exactly two arguments: port and max_clients
            std::cerr << "Usage: " << argv[0] << " <ip> <port>" << std::endl;
            return 1;
        }

        // Parse command-line arguments
        std::string ip = argv[1];
        int port = std::stoi(argv[2]);

        // Create and start the server
        game_server server(ip, port);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

