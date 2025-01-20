#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <thread>
#include <algorithm>
#include <random>
#include <stdexcept>
#include <vector>
#include <string>
#include "deck.h"
#include "card.h"
#include <mutex>
#include <map>
#include <chrono>
#include "json.h"
#include  <cstring>
#include "player.h"
#include "lobby.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define CLOSE_SOCKET closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #define CLOSE_SOCKET close
#endif


class game_server {
private:
#ifdef _WIN32
    SOCKET server_socket;
#else
    int server_socket;
#endif
    std::mutex mtx;
    std::vector<Lobby*> lobbies;
    std::map<std::string, Player*> players;
    bool running;
    static const int SHORT_DISCONNECT_THRESHOLD = 10; // 30 seconds for short disconnection
    static const int LONG_DISCONNECT_THRESHOLD = 60; // 2 minutes for long disconnection

    void handle_message(const SimpleJSON& msg, const sockaddr_in& client_addr);
    void send_to_client(SimpleJSON& msg, const sockaddr_in& client_addr);
    void broadcast_game_state(Lobby* lobby);
    void check_disconnections();
    Lobby* find_or_create_lobby(Player* player);
    Lobby* find_player_lobby(const std::string& player_name);
    std::string remove_quotes(const std::string& str);
    void handle_reconnection(const std::string& player_name, const sockaddr_in& new_addr);
    bool is_reconnection_valid(const std::string& player_name, const sockaddr_in& new_addr);
    void notify_reconnection(Player* player, Lobby* lobby);
    void notify_disconnection(Player* player, Lobby* lobby);

public:
    game_server(const std::string& ip, int port);
    void start();
    void stop();
    ~game_server();
};

#endif // SERVER_H